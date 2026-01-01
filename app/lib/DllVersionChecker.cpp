#include "DllVersionChecker.hpp"
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>

QStringList DllVersionChecker::getRequiredGgmlSymbols() {
    // Critical symbols that must be present in ggml.dll/llama.dll
    // based on llama.cpp version b7130 (commit 3f3a4fb9c from 2025-11-22)
    return {
        "ggml_init",
        "ggml_free",
        "ggml_new_tensor",
        "ggml_xielu",  // Added in llama.cpp in October/November 2025 - critical for new models
        "ggml_backend_init",
        "ggml_backend_free"
    };
}

QString DllVersionChecker::findDumpbinPath() {
    // Try to find dumpbin.exe in common Visual Studio locations
    QStringList candidates = {
        // VS 2022
        "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC",
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC",
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC",
        // VS 2019
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/VC/Tools/MSVC",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC",
    };

    for (const QString& baseDir : candidates) {
        QDir dir(baseDir);
        if (!dir.exists()) {
            continue;
        }

        // Look for the latest version directory
        QStringList versions = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
        for (const QString& version : versions) {
            QString dumpbinPath = QString("%1/%2/bin/Hostx64/x64/dumpbin.exe").arg(baseDir).arg(version);
            if (QFileInfo::exists(dumpbinPath)) {
                return dumpbinPath;
            }
        }
    }

    return QString();
}

bool DllVersionChecker::isDumpbinAvailable() {
    return !findDumpbinPath().isEmpty();
}

QStringList DllVersionChecker::extractExports(const QString& dumpbinOutput) {
    QStringList exports;
    
    // Parse dumpbin /EXPORTS output
    // Format: ordinal hint RVA      name
    // Example: 1    0 00001000 ggml_init
    QRegularExpression exportRegex(R"(^\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\w+))");
    
    QStringList lines = dumpbinOutput.split('\n');
    bool inExportSection = false;
    
    for (const QString& line : lines) {
        if (line.contains("ordinal hint")) {
            inExportSection = true;
            continue;
        }
        
        if (!inExportSection) {
            continue;
        }
        
        // Stop at summary section
        if (line.trimmed().startsWith("Summary")) {
            break;
        }
        
        QRegularExpressionMatch match = exportRegex.match(line);
        if (match.hasMatch()) {
            exports.append(match.captured(1));
        }
    }
    
    return exports;
}

DllVersionChecker::CheckResult DllVersionChecker::checkDllExports(
    const QString& dllPath, 
    const QStringList& requiredSymbols) 
{
    CheckResult result;
    
    if (!QFileInfo::exists(dllPath)) {
        result.errorMessage = QString("DLL not found: %1").arg(dllPath);
        return result;
    }
    
    QString dumpbinPath = findDumpbinPath();
    if (dumpbinPath.isEmpty()) {
        // Can't verify without dumpbin, assume compatible
        result.isCompatible = true;
        result.errorMessage = "Cannot verify DLL exports (dumpbin.exe not found)";
        qDebug() << "DllVersionChecker:" << result.errorMessage;
        return result;
    }
    
    QProcess process;
    process.setProgram(dumpbinPath);
    process.setArguments({"/EXPORTS", dllPath});
    process.start();
    
    if (!process.waitForStarted(3000)) {
        result.errorMessage = "Failed to start dumpbin.exe";
        qWarning() << "DllVersionChecker:" << result.errorMessage;
        result.isCompatible = true; // Assume compatible if we can't check
        return result;
    }
    
    if (!process.waitForFinished(10000)) {
        process.kill();
        result.errorMessage = "dumpbin.exe timed out";
        qWarning() << "DllVersionChecker:" << result.errorMessage;
        result.isCompatible = true; // Assume compatible if we can't check
        return result;
    }
    
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QStringList exports = extractExports(output);
    
    // Check for required symbols
    result.isCompatible = true;
    for (const QString& required : requiredSymbols) {
        if (!exports.contains(required)) {
            result.isCompatible = false;
            result.missingSymbols.append(required);
        }
    }
    
    if (!result.isCompatible) {
        result.errorMessage = QString("DLL is missing required exports: %1")
            .arg(result.missingSymbols.join(", "));
    }
    
    return result;
}

DllVersionChecker::CheckResult DllVersionChecker::checkLlamaDllCompatibility(const QString& dllPath) {
    QStringList requiredSymbols = getRequiredGgmlSymbols();
    CheckResult result = checkDllExports(dllPath, requiredSymbols);
    
    if (!result.isCompatible) {
        result.errorMessage += "\n\nThis usually means the llama.dll was built with an older version of llama.cpp.\n"
                               "The application requires llama.cpp version b7130 (2025-11-22) or later.";
    }
    
    return result;
}
