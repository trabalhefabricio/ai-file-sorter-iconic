#include "DllVersionChecker.hpp"
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <QtGlobal>
#include <cmath>

QStringList DllVersionChecker::getRequiredGgmlSymbols() {
    // Critical symbols that must be present in ggml.dll/llama.dll
    // based on llama.cpp version b7130 (commit 3f3a4fb9c from 2025-11-22)
    // 
    // ggml_xielu was added for the Apertus model support. Even if the Apertus model
    // is never used, the symbol must exist because llama.dll references it at load time.
    // Without it, Windows will refuse to load the DLL with "entry point not found" error.
    //
    // Note: We check for key functions across different modules to ensure compatibility:
    // - ggml_* functions: Core GGML operations
    // - gguf_* functions: GGUF file format support
    // - llama_* functions: High-level llama.cpp API
    return {
        // Core GGML functions
        "ggml_init",
        "ggml_free",
        "ggml_new_tensor",
        "ggml_backend_init",
        "ggml_backend_free",
        "ggml_xielu",  // Required since llama.cpp b7130 (Apertus model support)
        
        // GGUF format functions (used for loading model files)
        "gguf_init_from_file",
        "gguf_free",
        "gguf_get_n_tensors"
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

DllVersionChecker::CheckResult DllVersionChecker::checkQtRuntimeCompatibility() {
    CheckResult result;
    result.isCompatible = true; // Assume compatible unless proven otherwise
    
    // Check Qt version at runtime vs compile time
    QString runtimeVersion = QString::fromLatin1(qVersion());
    QString compileVersion = QString::fromLatin1(QT_VERSION_STR);
    
    result.dllVersion = QString("Runtime: %1, Compile-time: %2").arg(runtimeVersion).arg(compileVersion);
    
    // Major version must match
    QStringList runtimeParts = runtimeVersion.split('.');
    QStringList compileParts = compileVersion.split('.');
    
    if (runtimeParts.isEmpty() || compileParts.isEmpty()) {
        result.errorMessage = QString("Unable to parse Qt versions. Runtime: %1, Compile: %2")
            .arg(runtimeVersion).arg(compileVersion);
        result.isCompatible = false;
        return result;
    }
    
    bool runtimeMajorOk = false;
    bool compileMajorOk = false;
    int runtimeMajor = runtimeParts[0].toInt(&runtimeMajorOk);
    int compileMajor = compileParts[0].toInt(&compileMajorOk);
    
    if (!runtimeMajorOk || !compileMajorOk) {
        result.errorMessage = QString("Failed to parse Qt version numbers. Runtime: %1, Compile: %2")
            .arg(runtimeVersion).arg(compileVersion);
        result.isCompatible = false;
        return result;
    }
    
    if (runtimeMajor != compileMajor) {
        result.isCompatible = false;
        result.errorMessage = QString(
            "Qt major version mismatch!\n\n"
            "Application was built with Qt %1\n"
            "But runtime is using Qt %2\n\n"
            "This causes \"entry point not found\" errors like:\n"
            "- QTableView::dropEvent not found\n"
            "- Other Qt virtual function errors\n\n"
            "Solutions:\n"
            "1. Ensure Qt %3 runtime DLLs are in your PATH or application directory\n"
            "2. Reinstall Qt %3 runtime libraries\n"
            "3. Remove conflicting Qt versions from your PATH"
        ).arg(compileVersion).arg(runtimeVersion).arg(compileMajor);
        return result;
    }
    
    // Warn if minor versions differ significantly
    constexpr int MAX_ALLOWED_MINOR_VERSION_DIFF = 2;
    if (runtimeParts.size() >= 2 && compileParts.size() >= 2) {
        bool runtimeMinorOk = false;
        bool compileMinorOk = false;
        int runtimeMinor = runtimeParts[1].toInt(&runtimeMinorOk);
        int compileMinor = compileParts[1].toInt(&compileMinorOk);
        
        if (runtimeMinorOk && compileMinorOk && 
            std::abs(runtimeMinor - compileMinor) > MAX_ALLOWED_MINOR_VERSION_DIFF) {
            result.errorMessage = QString(
                "Qt minor version difference detected:\n"
                "Built with Qt %1, running with Qt %2\n\n"
                "This may cause compatibility issues with virtual functions.\n"
                "Consider using the same minor version."
            ).arg(compileVersion).arg(runtimeVersion);
            // Still compatible, just a warning
            qWarning().noquote() << "DllVersionChecker:" << result.errorMessage;
        }
    }
    
    qInfo().noquote() << "Qt version check passed:" << result.dllVersion;
    return result;
}
