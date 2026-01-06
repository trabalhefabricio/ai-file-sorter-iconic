#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QLibrary>
#include <QDesktopServices>
#include <QUrl>
#include <QByteArray>
#include <QObject>
#include <QStringList>

#include "DllVersionChecker.hpp"
#include "ErrorReporter.hpp"

#include <cstdlib>
#include <string>

#include <windows.h>
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif
// DLL search order flags for SetDefaultDllDirectories
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS 0x00001000
#endif
using SetProcessDpiAwarenessContextFn = BOOL (WINAPI *)(HANDLE);
using SetProcessDpiAwarenessFn = HRESULT (WINAPI *)(int); // 2 = PROCESS_PER_MONITOR_DPI_AWARE

namespace {

enum class BackendOverride {
    None,
    ForceOn,
    ForceOff
};

enum class BackendSelection {
    Cpu,
    Cuda,
    Vulkan
};

BackendOverride parseBackendOverride(QString value) {
    value = value.trimmed().toLower();
    if (value == QLatin1String("on")) {
        return BackendOverride::ForceOn;
    }
    if (value == QLatin1String("off")) {
        return BackendOverride::ForceOff;
    }
    return BackendOverride::None;
}

bool enableSecureDllSearch()
{
    // Use LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_APPLICATION_DIR
    // to ensure application directory is prioritized over system PATH
    const DWORD searchFlags = LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_APPLICATION_DIR;
    
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0602
    return SetDefaultDllDirectories(searchFlags) != 0;
#else
    // Only available on Windows 7+ with KB2533623. Try to enable if present.
    typedef BOOL (WINAPI *SetDefaultDllDirectoriesFunc)(DWORD);
    if (const HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll")) {
        if (const auto fn = reinterpret_cast<SetDefaultDllDirectoriesFunc>(
                GetProcAddress(kernel32, "SetDefaultDllDirectories"))) {
            return fn(searchFlags) != 0;
        }
    }
    return false;
#endif
}

void addDllDirectoryChecked(const QString& directory)
{
    if (directory.isEmpty()) {
        return;
    }
    const std::wstring wideDir = QDir::toNativeSeparators(directory).toStdWString();
    if (AddDllDirectory(wideDir.c_str()) == nullptr) {
        qWarning().noquote()
            << "AddDllDirectory failed for"
            << QDir::toNativeSeparators(directory)
            << "- error" << GetLastError();
    } else {
        qInfo().noquote()
            << "Registered DLL directory"
            << QDir::toNativeSeparators(directory);
    }
}

bool tryLoadLibrary(const QString& name) {
    QLibrary lib(name);
    const bool loaded = lib.load();
    if (loaded) {
        lib.unload();
    }
    return loaded;
}

QStringList candidateGgmlDirectories(const QString& exeDir, const QString& variant)
{
    QStringList candidates;
    candidates << QDir(exeDir).filePath(QStringLiteral("lib/ggml/%1").arg(variant));
    candidates << QDir(exeDir).filePath(QStringLiteral("ggml/%1").arg(variant));
    return candidates;
}

const QList<int>& knownCudaRuntimeVersions()
{
    static const QList<int> versions = {
        75, 80, 90, 91, 92,      // CUDA 7.5-9.2
        100, 101, 102,           // CUDA 10.x
        110, 111, 112, 113, 114, 115, 116, 117, 118, // CUDA 11.x variants
        120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130 // CUDA 12.x variants
    };
    return versions;
}

const QList<int>& requiredCudaRuntimeVersions()
{
    static const QList<int> versions = { 120 }; // keep in sync with build script (CUDA 12.x)
    return versions;
}

bool isCudaRuntimePresent(const QList<int>& versions, QString *loadedRuntime = nullptr)
{
    for (int version : versions) {
        const QString runtime = QStringLiteral("cudart64_%1").arg(version);
        if (tryLoadLibrary(runtime)) {
            if (loadedRuntime) {
                *loadedRuntime = runtime;
            }
            return true;
        }
    }
    return false;
}

bool isCudaAvailable(QString *loadedRuntime = nullptr) {
    return isCudaRuntimePresent(knownCudaRuntimeVersions(), loadedRuntime);
}

bool isRequiredCudaRuntimePresent(QString *loadedRuntime = nullptr) {
    return isCudaRuntimePresent(requiredCudaRuntimeVersions(), loadedRuntime);
}

bool loadVulkanLibrary(const QString& path) {
    const std::wstring native = QDir::toNativeSeparators(path).toStdWString();
    HMODULE module = LoadLibraryW(native.c_str());
    if (!module) {
        return false;
    }
    FreeLibrary(module);
    return true;
}

bool isVulkanRuntimeAvailable(const QString& exeDir) {
    if (loadVulkanLibrary(QStringLiteral("vulkan-1.dll"))) {
        qInfo().noquote() << "Detected system Vulkan runtime via PATH.";
        return true;
    }

    const QStringList bundledCandidates = {
        QDir(exeDir).filePath(QStringLiteral("lib/precompiled/vulkan/bin/vulkan-1.dll")),
    };

    QStringList ggmlCandidates = candidateGgmlDirectories(exeDir, QStringLiteral("wvulkan"));
    for (QString& root : ggmlCandidates) {
        root = QDir(root).filePath(QStringLiteral("vulkan-1.dll"));
    }

    for (const QString& candidate : bundledCandidates + ggmlCandidates) {
        if (QFileInfo::exists(candidate)) {
            qInfo().noquote()
                << "Detected bundled Vulkan runtime at"
                << QDir::toNativeSeparators(candidate);
            return true;
        }
    }

    return false;
}

bool isNvidiaDriverAvailable() {
    static const QStringList driverCandidates = {
        QStringLiteral("nvml"),
        QStringLiteral("nvcuda"),
        QStringLiteral("nvapi64")
    };

    for (const QString& dll : driverCandidates) {
        if (tryLoadLibrary(dll)) {
            return true;
        }
    }
    return false;
}

void appendToProcessPath(const QString& directory) {
    if (directory.isEmpty()) {
        return;
    }

    QByteArray path = qgetenv("PATH");
    if (!path.isEmpty()) {
        path.append(';');
    }
    path.append(QDir::toNativeSeparators(directory).toUtf8());
    qputenv("PATH", path);
    qInfo().noquote() << "Added to PATH:" << QDir::toNativeSeparators(directory);
    qInfo().noquote() << "Current PATH:" << QString::fromUtf8(qgetenv("PATH"));
}

bool promptCudaDownload() {
    const auto response = QMessageBox::warning(
        nullptr,
        QObject::tr("CUDA Toolkit Missing"),
        QObject::tr("A compatible NVIDIA GPU was detected, but the CUDA Toolkit is missing.\n\n"
                    "CUDA is required for GPU acceleration in this application.\n\n"
                    "Would you like to download and install it now?"),
        QMessageBox::Ok | QMessageBox::Cancel,
        QMessageBox::Ok);

    if (response == QMessageBox::Ok) {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://developer.nvidia.com/cuda-downloads")));
        return true;
    }
    return false;
}

bool launchMainExecutable(const QString& executablePath,
                          const QStringList& arguments,
                          bool disableCuda,
                          const QString& backendTag,
                          const QString& ggmlDir,
                          const QString& llamaDevice) {
    QFileInfo exeInfo(executablePath);
    if (!exeInfo.exists()) {
        qCritical().noquote() << "Main executable not found:" << QDir::toNativeSeparators(executablePath);
        qCritical().noquote() << "The application cannot start without the main executable file.";
        qCritical().noquote() << "Please verify the installation is complete and not corrupted.";
        return false;
    }

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PATH"), QString::fromUtf8(qgetenv("PATH")));
    environment.insert(QStringLiteral("GGML_DISABLE_CUDA"), disableCuda ? QStringLiteral("1") : QStringLiteral("0"));
    environment.insert(QStringLiteral("AI_FILE_SORTER_GPU_BACKEND"), backendTag);
    environment.insert(QStringLiteral("AI_FILE_SORTER_GGML_DIR"), ggmlDir);
    environment.insert(QStringLiteral("LLAMA_ARG_DEVICE"), llamaDevice);

    QProcess process;
    process.setProcessEnvironment(environment);
    process.setProgram(executablePath);
    process.setArguments(arguments);
    process.setWorkingDirectory(exeInfo.absolutePath());

    qint64 pid = 0;
    bool started = process.startDetached(&pid);
    
    if (!started) {
        qCritical().noquote() << "Failed to start detached process for:" << QDir::toNativeSeparators(executablePath);
        qCritical().noquote() << "Process error:" << process.errorString();
        qCritical().noquote() << "This may be caused by:";
        qCritical().noquote() << "  - Missing dependencies (DLLs)";
        qCritical().noquote() << "  - Insufficient permissions";
        qCritical().noquote() << "  - Antivirus blocking execution";
        qCritical().noquote() << "  - Corrupted executable file";
        return false;
    }
    
    qInfo().noquote() << "Successfully launched main application process with PID:" << pid;
    return true;
}

QString resolveExecutableName(const QString& baseDir) {
    const QStringList candidates = {
        QStringLiteral("aifilesorter.exe"),
        QStringLiteral("AI File Sorter.exe")
    };

    for (const QString& candidate : candidates) {
        const QString fullPath = QDir(baseDir).filePath(candidate);
        if (QFileInfo::exists(fullPath)) {
            qInfo().noquote() << "Found main executable:" << QDir::toNativeSeparators(fullPath);
            return fullPath;
        }
    }

    // None of the candidates exist - log error and return empty to signal failure
    qCritical().noquote() << "Main executable not found in:" << QDir::toNativeSeparators(baseDir);
    qCritical().noquote() << "Searched for:";
    for (const QString& candidate : candidates) {
        qCritical().noquote() << "  -" << candidate;
    }
    qCritical().noquote() << "Please verify the application installation is complete.";
    
    return QString(); // Return empty string to signal error
}

struct BackendOverrides {
    BackendOverride cuda{BackendOverride::None};
    BackendOverride vulkan{BackendOverride::None};
    QStringList observedArgs;
};

struct BackendAvailability {
    bool hasNvidiaDriver{false};
    bool cudaRuntimeDetected{false};
    bool runtimeCompatible{false};
    bool cudaAvailable{false};
    bool vulkanAvailable{false};
    bool cudaInitiallyAvailable{false};
    bool vulkanInitiallyAvailable{false};
    QString detectedCudaRuntime;
};

BackendOverrides parse_backend_overrides(int argc, char* argv[])
{
    BackendOverrides overrides;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        overrides.observedArgs << arg;
        if (arg.startsWith(QStringLiteral("--cuda="))) {
            overrides.cuda = parseBackendOverride(arg.mid(7));
        } else if (arg.startsWith(QStringLiteral("--vulkan="))) {
            overrides.vulkan = parseBackendOverride(arg.mid(9));
        }
    }
    return overrides;
}

void log_observed_arguments(const QStringList& args)
{
    if (args.isEmpty()) {
        return;
    }
    qInfo().noquote() << "Starter arguments:" << args.join(QLatin1Char(' '));
}

bool maybe_prompt_cuda_download(const BackendOverrides& overrides,
                                const BackendAvailability& availability)
{
    if (!availability.hasNvidiaDriver) {
        return false;
    }

    const bool runtimeMissing = !availability.cudaRuntimeDetected;
    const bool runtimeIncompatible = availability.cudaRuntimeDetected && !availability.runtimeCompatible;
    if (!runtimeMissing && !runtimeIncompatible) {
        return false;
    }
    if (overrides.cuda == BackendOverride::ForceOff) {
        return false;
    }

    const bool cudaRequested = overrides.cuda == BackendOverride::ForceOn;
    const bool vulkanUnavailable = !availability.vulkanAvailable;
    if (!cudaRequested && !vulkanUnavailable) {
        return false;
    }

    return promptCudaDownload();
}

bool validate_override_conflict(const BackendOverrides& overrides)
{
    if (overrides.cuda == BackendOverride::ForceOn &&
        overrides.vulkan == BackendOverride::ForceOn) {
        QMessageBox::critical(nullptr,
                              QObject::tr("Launch Error"),
                              QObject::tr("Cannot enable both CUDA and Vulkan simultaneously."));
        return false;
    }
    return true;
}

BackendAvailability detect_backend_availability(const QString& exeDir,
                                                bool hasNvidiaDriver,
                                                bool cudaRuntimeDetected,
                                                const QString& detectedRuntimeName)
{
    BackendAvailability availability;
    availability.hasNvidiaDriver = hasNvidiaDriver;
    availability.cudaRuntimeDetected = cudaRuntimeDetected;
    QString compatibleRuntime;
    availability.runtimeCompatible = isRequiredCudaRuntimePresent(&compatibleRuntime);
    availability.detectedCudaRuntime = availability.runtimeCompatible ? compatibleRuntime : detectedRuntimeName;
    availability.cudaAvailable = availability.runtimeCompatible && hasNvidiaDriver;
    availability.vulkanAvailable = isVulkanRuntimeAvailable(exeDir);
    availability.cudaInitiallyAvailable = availability.cudaAvailable;
    availability.vulkanInitiallyAvailable = availability.vulkanAvailable;
    if (hasNvidiaDriver && cudaRuntimeDetected && !availability.runtimeCompatible) {
        const QString requiredRuntime = QStringLiteral("cudart64_%1.dll").arg(requiredCudaRuntimeVersions().constFirst());
        qWarning().noquote()
            << "Detected CUDA runtime"
            << (availability.detectedCudaRuntime.isEmpty() ? QStringLiteral("<unknown>") : availability.detectedCudaRuntime)
            << "but the bundled GGML build requires"
            << requiredRuntime << "."
            << "Falling back to alternate backend.";
    }
    return availability;
}

void apply_override_flags(const BackendOverrides& overrides,
                          BackendAvailability& availability)
{
    if (overrides.cuda == BackendOverride::ForceOff) {
        availability.cudaAvailable = false;
        qInfo().noquote() << "CUDA manually disabled via --cuda=off.";
    }
    if (overrides.vulkan == BackendOverride::ForceOff) {
        availability.vulkanAvailable = false;
        qInfo().noquote() << "Vulkan manually disabled via --vulkan=off.";
    }
}

BackendSelection resolve_backend_selection(const BackendOverrides& overrides,
                                           const BackendAvailability& availability)
{
    BackendSelection selection = BackendSelection::Cpu;
    if (overrides.vulkan == BackendOverride::ForceOn) {
        if (availability.vulkanAvailable) {
            return BackendSelection::Vulkan;
        }
        qWarning().noquote() << "Vulkan forced but not detected; ignoring request.";
    }
    if (overrides.cuda == BackendOverride::ForceOn) {
        if (availability.cudaAvailable) {
            return BackendSelection::Cuda;
        }
        qWarning().noquote() << "CUDA forced but not detected; ignoring request.";
    }
    if (availability.vulkanAvailable) {
        return BackendSelection::Vulkan;
    }
    if (availability.cudaAvailable) {
        return BackendSelection::Cuda;
    }
    return selection;
}

QString incompatible_runtime_message(const BackendAvailability& availability)
{
    if (availability.cudaRuntimeDetected && !availability.runtimeCompatible) {
        return QStringLiteral("CUDA runtime ignored due to incompatibility; using CPU backend.");
    }
    return QStringLiteral("No GPU runtime detected; using CPU backend.");
}

QString cpu_backend_message(const BackendAvailability& availability)
{
    if (!availability.cudaAvailable && !availability.vulkanAvailable) {
        return incompatible_runtime_message(availability);
    }
    if (availability.cudaInitiallyAvailable && !availability.cudaAvailable) {
        return QStringLiteral("CUDA runtime ignored due to override; using CPU backend.");
    }
    if (availability.vulkanInitiallyAvailable && !availability.vulkanAvailable) {
        return QStringLiteral("Vulkan runtime ignored due to override; using CPU backend.");
    }
    return QStringLiteral("CUDA and Vulkan explicitly disabled; using CPU backend.");
}

void enable_per_monitor_dpi_awareness()
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        const auto set_ctx = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (set_ctx && set_ctx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            return;
        }
    }
    HMODULE shcore = LoadLibraryW(L"Shcore.dll");
    if (shcore) {
        const auto set_awareness = reinterpret_cast<SetProcessDpiAwarenessFn>(
            GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (set_awareness) {
            set_awareness(2); // PROCESS_PER_MONITOR_DPI_AWARE
        }
        FreeLibrary(shcore);
    }
}

void log_runtime_availability(const BackendAvailability& availability,
                              BackendSelection selection)
{
    const QString availabilityLine =
        QStringLiteral("Runtime availability: CUDA=%1 Vulkan=%2")
            .arg(availability.cudaInitiallyAvailable ? QStringLiteral("yes") : QStringLiteral("no"))
            .arg(availability.vulkanInitiallyAvailable ? QStringLiteral("yes") : QStringLiteral("no"));
    qInfo().noquote() << availabilityLine;

    switch (selection) {
        case BackendSelection::Vulkan:
            qInfo().noquote() << "Backend selection: Vulkan (priority order Vulkan → CUDA → CPU).";
            break;
        case BackendSelection::Cuda:
            qInfo().noquote() << "Backend selection: CUDA (Vulkan unavailable).";
            break;
        case BackendSelection::Cpu:
        default:
            qInfo().noquote() << cpu_backend_message(availability);
            break;
    }
}

QString ggml_variant_for_selection(BackendSelection selection)
{
    switch (selection) {
        case BackendSelection::Cuda:
            return QStringLiteral("wcuda");
        case BackendSelection::Vulkan:
            return QStringLiteral("wvulkan");
        case BackendSelection::Cpu:
        default:
            return QStringLiteral("wocuda");
    }
}

QString resolve_ggml_directory(const QString& exeDir,
                               const QString& variant,
                               bool showError = true)
{
    const QStringList candidates = candidateGgmlDirectories(exeDir, variant);
    for (const QString& candidate : candidates) {
        if (QDir(candidate).exists()) {
            if (candidate != candidates.front()) {
                qInfo().noquote() << "Primary GGML directory missing; using fallback"
                                  << QDir::toNativeSeparators(candidate);
            }
            return candidate;
        }
    }

    if (showError) {
        QMessageBox::critical(
            nullptr,
            QObject::tr("Missing GGML Runtime"),
            QObject::tr("Could not locate the backend runtime DLLs.\nTried:\n%1\n%2")
                .arg(QDir::toNativeSeparators(candidates.value(0)),
                     QDir::toNativeSeparators(candidates.value(1))));
    }
    return QString();
}

void configure_runtime_paths(const QString& exeDir,
                             const QString& ggmlPath,
                             bool secureSearchEnabled,
                             bool useCuda,
                             bool useVulkan)
{
    appendToProcessPath(ggmlPath);
    if (secureSearchEnabled) {
        addDllDirectoryChecked(ggmlPath);
    }

    QStringList additionalDllRoots;
    additionalDllRoots << QDir(exeDir).filePath(QStringLiteral("lib/precompiled/cpu/bin"));
    if (useCuda) {
        additionalDllRoots << QDir(exeDir).filePath(QStringLiteral("lib/precompiled/cuda/bin"));
    }
    if (useVulkan) {
        additionalDllRoots << QDir(exeDir).filePath(QStringLiteral("lib/precompiled/vulkan/bin"));
    }
    additionalDllRoots << QDir(exeDir).filePath(QStringLiteral("bin"));
    additionalDllRoots << exeDir;
    for (const QString& dir : additionalDllRoots) {
        if (!QDir(dir).exists()) {
            continue;
        }
        appendToProcessPath(dir);
        if (secureSearchEnabled) {
            addDllDirectoryChecked(dir);
        }
    }
}

QStringList build_forwarded_args(int argc, char* argv[], bool &console_log_flag)
{
    QStringList forwardedArgs;
    console_log_flag = false;
    
    // List of flag prefixes that should not be forwarded (handled by starter only)
    static const QStringList excludedPrefixes = {
        QStringLiteral("--cuda="),
        QStringLiteral("--vulkan=")
    };
    
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        
        // Check for console-log flag
        if (arg == QStringLiteral("--console-log")) {
            console_log_flag = true;
        }
        
        // Skip backend override flags - they're for the starter, not the main app
        bool shouldExclude = false;
        for (const QString& prefix : excludedPrefixes) {
            if (arg.startsWith(prefix)) {
                shouldExclude = true;
                break;
            }
        }
        
        if (!shouldExclude) {
            forwardedArgs.append(arg);
        }
    }
    
    // Always add --allow-direct-launch to indicate app was launched via starter
    forwardedArgs.prepend(QStringLiteral("--allow-direct-launch"));
    
    return forwardedArgs;
}

QString backend_tag_for_selection(BackendSelection selection)
{
    switch (selection) {
        case BackendSelection::Cuda: return QStringLiteral("cuda");
        case BackendSelection::Vulkan: return QStringLiteral("vulkan");
        case BackendSelection::Cpu:
        default: return QStringLiteral("cpu");
    }
}

QString llama_device_for_selection(BackendSelection selection)
{
    switch (selection) {
        case BackendSelection::Cuda: return QStringLiteral("cuda");
        case BackendSelection::Vulkan: return QStringLiteral("vulkan");
        case BackendSelection::Cpu:
        default: return QString();
    }
}

bool check_dll_compatibility(const QString& ggmlPath, const QString& exeDir) {
    // First, check Qt runtime version compatibility
    qInfo() << "Checking Qt runtime version compatibility...";
    DllVersionChecker::CheckResult qtResult = DllVersionChecker::checkQtRuntimeCompatibility();
    
    if (!qtResult.isCompatible) {
        qWarning().noquote() << "Qt version mismatch detected:" << qtResult.errorMessage;
        
        QString qtErrorPrompt = QObject::tr(
            "%1\n\nDo you want to continue anyway? (Not recommended)"
        ).arg(qtResult.errorMessage);
        
        auto response = QMessageBox::critical(
            nullptr,
            QObject::tr("Qt Version Mismatch"),
            qtErrorPrompt,
            QMessageBox::Ignore | QMessageBox::Abort,
            QMessageBox::Abort
        );
        
        if (response == QMessageBox::Abort) {
            return false;
        }
    } else {
        qInfo().noquote() << "Qt version check passed:" << qtResult.dllVersion;
    }
    
    // Check llama.dll and ggml.dll for required exports
    QStringList dllsToCheck;
    
    // Check in the ggml runtime directory
    if (!ggmlPath.isEmpty()) {
        dllsToCheck << QDir(ggmlPath).filePath("llama.dll");
        dllsToCheck << QDir(ggmlPath).filePath("ggml.dll");
    }
    
    // Check in precompiled/cpu/bin
    QString precompiledDir = QDir(exeDir).filePath("lib/precompiled/cpu/bin");
    dllsToCheck << QDir(precompiledDir).filePath("llama.dll");
    dllsToCheck << QDir(precompiledDir).filePath("ggml.dll");
    
    bool foundAnyDll = false;
    bool hasIncompatibleDll = false;
    QStringList incompatibleDlls;
    QStringList missingSymbols;
    
    for (const QString& dllPath : dllsToCheck) {
        if (!QFileInfo::exists(dllPath)) {
            continue;
        }
        
        foundAnyDll = true;
        qInfo().noquote() << "Checking DLL compatibility:" << QDir::toNativeSeparators(dllPath);
        
        DllVersionChecker::CheckResult result = DllVersionChecker::checkLlamaDllCompatibility(dllPath);
        
        if (!result.isCompatible && !result.missingSymbols.isEmpty()) {
            hasIncompatibleDll = true;
            incompatibleDlls.append(QFileInfo(dllPath).fileName());
            for (const QString& symbol : result.missingSymbols) {
                if (!missingSymbols.contains(symbol)) {
                    missingSymbols.append(symbol);
                }
            }
            qWarning().noquote() << "DLL version mismatch detected:" << result.errorMessage;
        } else if (result.isCompatible) {
            qInfo().noquote() << "DLL compatibility check passed for" << QFileInfo(dllPath).fileName();
        }
    }
    
    if (!foundAnyDll) {
        qWarning() << "No llama/ggml DLLs found to check";
        return true; // Can't check, proceed anyway
    }
    
    if (hasIncompatibleDll) {
        QString message = QString(
            "DLL Version Mismatch Detected\n\n"
            "The following DLL(s) are outdated and missing required functions:\n"
            "%1\n\n"
            "Missing exports: %2\n\n"
            "This will cause \"entry point not found\" errors at runtime.\n\n"
            "Common errors caused by this mismatch:\n"
            "- \"Could not locate the entry point for procedure ggml_xielu\"\n"
            "- Application fails to start with DLL errors\n\n"
            "Solutions:\n"
            "1. If you built from source: Rebuild llama.dll using:\n"
            "   app\\scripts\\build_llama_windows.ps1\n\n"
            "2. If using prebuilt binaries: Download the latest version\n\n"
            "Do you want to continue anyway? (Not recommended)"
        ).arg(incompatibleDlls.join(", ")).arg(missingSymbols.join(", "));
        
        auto response = QMessageBox::warning(
            nullptr,
            QObject::tr("DLL Version Mismatch"),
            message,
            QMessageBox::Ignore | QMessageBox::Abort,
            QMessageBox::Abort
        );
        
        return (response == QMessageBox::Ignore);
    }
    
    return true;
}

bool launch_main_process(const QString& mainExecutable,
                         const QStringList& forwardedArgs,
                         BackendSelection selection,
                         const QString& ggmlPath)
{
    const bool disableCudaEnv = (selection != BackendSelection::Cuda);
    const QString backendTag = backend_tag_for_selection(selection);
    const QString llamaDevice = llama_device_for_selection(selection);
    if (!launchMainExecutable(mainExecutable,
                              forwardedArgs,
                              disableCudaEnv,
                              backendTag,
                              ggmlPath,
                              llamaDevice)) {
        QMessageBox::critical(nullptr,
            QObject::tr("Launch Failed"),
            QObject::tr("Failed to launch the main application executable:\n%1").arg(mainExecutable));
        return false;
    }
    return true;
}

/**
 * @brief Show a detailed, copyable error dialog with diagnostic information
 * 
 * This creates a QMessageBox with DetailedText that can be copied to clipboard.
 * Users can click "Show Details" to see and copy the full diagnostic information.
 */
void showDllSetupError(const QString& summary, const QString& details) {
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle(QObject::tr("Critical DLL Setup Error"));
    msgBox.setText(summary);
    msgBox.setDetailedText(details);
    msgBox.setStandardButtons(QMessageBox::Abort | QMessageBox::Ignore);
    msgBox.setDefaultButton(QMessageBox::Abort);
    
    // Make the detailed text copyable
    msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    
    msgBox.exec();
}

/**
 * @brief Get detailed PATH information for diagnostics
 */
QString getPathDiagnostics() {
    QString diagnostics;
    
    #ifdef _WIN32
    wchar_t pathBuffer[32768];
    DWORD pathSize = GetEnvironmentVariableW(L"PATH", pathBuffer, 32768);
    
    if (pathSize > 0 && pathSize < 32768) {
        QString pathStr = QString::fromWCharArray(pathBuffer);
        QStringList paths = pathStr.split(';', Qt::SkipEmptyParts);
        
        diagnostics += "System PATH Directories (first 10):\n";
        for (int i = 0; i < qMin(10, paths.size()); ++i) {
            diagnostics += QString("  %1. %2\n").arg(i + 1).arg(paths[i]);
        }
        
        if (paths.size() > 10) {
            diagnostics += QString("  ... and %1 more directories\n").arg(paths.size() - 10);
        }
        
        // Look for Qt installations in PATH
        diagnostics += "\nQt installations found in PATH:\n";
        bool foundQt = false;
        for (const QString& path : paths) {
            if (path.contains("Qt", Qt::CaseInsensitive) || 
                QFileInfo::exists(path + "/Qt6Core.dll") ||
                QFileInfo::exists(path + "/Qt6Widgets.dll")) {
                diagnostics += QString("  - %1\n").arg(path);
                foundQt = true;
            }
        }
        if (!foundQt) {
            diagnostics += "  (None found)\n";
        }
    }
    #endif
    
    return diagnostics;
}

} // namespace

int main(int argc, char* argv[]) {
    enable_per_monitor_dpi_awareness();
    
    // CRITICAL: Set up DLL search paths BEFORE creating QApplication
    // This prevents loading incompatible Qt DLLs from system PATH
    const bool secureSearchEnabled = enableSecureDllSearch();
    
    // Get exe directory using Win32 API (before Qt is initialized)
    wchar_t exePath[MAX_PATH * 2]; // Use larger buffer to handle long paths
    DWORD pathLen = GetModuleFileNameW(NULL, exePath, MAX_PATH * 2);
    
    bool dllPathSetupSuccessful = false;
    if (pathLen > 0 && pathLen < MAX_PATH * 2) {
        std::wstring exePathStr(exePath);
        size_t lastSlash = exePathStr.find_last_of(L"\\/");
        std::wstring exeDirW = (lastSlash != std::wstring::npos) ? exePathStr.substr(0, lastSlash) : exePathStr;
        
        if (secureSearchEnabled && !exeDirW.empty()) {
            // Add application directory to DLL search path FIRST (before Qt loads)
            // This is CRITICAL to prevent loading Qt DLLs from system PATH
            if (AddDllDirectory(exeDirW.c_str()) == nullptr) {
                DWORD error = GetLastError();
                // Show error but continue - we'll try PATH fallback
                std::wstring errorMsg = L"Failed to add application directory to DLL search path (error ";
                errorMsg += std::to_wstring(error);
                errorMsg += L"). This may cause Qt version mismatch errors.";
                MessageBoxW(NULL, errorMsg.c_str(), L"DLL Setup Warning", MB_ICONWARNING | MB_OK);
            } else {
                dllPathSetupSuccessful = true;
            }
            
            // Also add bin subdirectory if it exists
            std::wstring binDir = exeDirW + L"\\bin";
            if (GetFileAttributesW(binDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
                if (AddDllDirectory(binDir.c_str()) == nullptr) {
                    DWORD error = GetLastError();
                    // Warn but continue
                    std::wstring errorMsg = L"Failed to add bin directory to DLL search path (error ";
                    errorMsg += std::to_wstring(error);
                    errorMsg += L").";
                    MessageBoxW(NULL, errorMsg.c_str(), L"DLL Setup Warning", MB_ICONWARNING | MB_OK);
                }
            }
        }
        
        // Always set PATH as a fallback, even if secure search is enabled
        // This ensures maximum compatibility
        if (!exeDirW.empty()) {
            // Prepend application directory to PATH to ensure local Qt DLLs are found first
            // This is CRITICAL for preventing Qt version mismatch errors
            constexpr DWORD PATH_BUFFER_SIZE = 32768;  // Maximum PATH length on Windows
            constexpr DWORD EXTRA_PATH_SPACE = 10;      // Extra space for separators
            wchar_t pathBuffer[PATH_BUFFER_SIZE];
            DWORD pathSize = GetEnvironmentVariableW(L"PATH", pathBuffer, PATH_BUFFER_SIZE);
            
            if (pathSize > 0 && pathSize < PATH_BUFFER_SIZE) {
                // Build new PATH with application directories prepended
                std::wstring binDir = exeDirW + L"\\bin";
                bool hasBinDir = (GetFileAttributesW(binDir.c_str()) != INVALID_FILE_ATTRIBUTES);
                
                // Reserve space for efficiency
                std::wstring newPath;
                newPath.reserve(pathSize + exeDirW.length() + (hasBinDir ? binDir.length() + 2 : 1) + EXTRA_PATH_SPACE);
                
                if (hasBinDir) {
                    newPath = binDir + L";" + exeDirW + L";" + std::wstring(pathBuffer);
                } else {
                    newPath = exeDirW + L";" + std::wstring(pathBuffer);
                }
                
                if (!SetEnvironmentVariableW(L"PATH", newPath.c_str())) {
                    DWORD error = GetLastError();
                    std::wstring errorMsg = L"Failed to set PATH environment variable (error ";
                    errorMsg += std::to_wstring(error);
                    errorMsg += L"). This may cause DLL loading errors.";
                    MessageBoxW(NULL, errorMsg.c_str(), L"PATH Setup Warning", MB_ICONWARNING | MB_OK);
                } else {
                    dllPathSetupSuccessful = true;
                }
            } else if (pathSize == 0) {
                // PATH not set or empty, just set to our directory
                std::wstring binDir = exeDirW + L"\\bin";
                BOOL success;
                if (GetFileAttributesW(binDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    success = SetEnvironmentVariableW(L"PATH", (binDir + L";" + exeDirW).c_str());
                } else {
                    success = SetEnvironmentVariableW(L"PATH", exeDirW.c_str());
                }
                if (!success) {
                    DWORD error = GetLastError();
                    std::wstring errorMsg = L"Failed to set PATH environment variable (error ";
                    errorMsg += std::to_wstring(error);
                    errorMsg += L"). This may cause DLL loading errors.";
                    MessageBoxW(NULL, errorMsg.c_str(), L"PATH Setup Warning", MB_ICONWARNING | MB_OK);
                } else {
                    dllPathSetupSuccessful = true;
                }
            } else if (pathSize >= PATH_BUFFER_SIZE) {
                // PATH is too large - show warning
                MessageBoxW(NULL, 
                    L"System PATH is too large to modify. This may cause DLL version conflicts.\n\n"
                    L"Consider removing unnecessary entries from your system PATH.",
                    L"PATH Setup Warning", MB_ICONWARNING | MB_OK);
            }
        }
    }
    
    // Store DLL setup status and diagnostics BEFORE creating QApplication
    // We'll show a detailed error dialog after Qt is initialized
    bool needsDllSetupWarning = (!dllPathSetupSuccessful && pathLen > 0);
    QString dllSetupDiagnostics;
    
    if (needsDllSetupWarning) {
        // Collect diagnostic information while we still have the raw Windows data
        std::wstring exePathStr(exePath);
        size_t lastSlash = exePathStr.find_last_of(L"\\/");
        std::wstring exeDirW = (lastSlash != std::wstring::npos) ? exePathStr.substr(0, lastSlash) : exePathStr;
        
        dllSetupDiagnostics += "=== DLL Setup Diagnostics ===\n\n";
        dllSetupDiagnostics += QString("Application Directory: %1\n\n")
            .arg(QString::fromWCharArray(exeDirW.c_str()));
        
        dllSetupDiagnostics += "DLL Setup Methods Attempted:\n";
        dllSetupDiagnostics += QString("  - AddDllDirectory: %1\n")
            .arg(secureSearchEnabled ? "Attempted (failed)" : "Not available");
        dllSetupDiagnostics += "  - PATH prepending: Attempted (failed)\n\n";
        
        dllSetupDiagnostics += "This failure means the system may load Qt DLLs from:\n";
        dllSetupDiagnostics += "  - System PATH (wrong version)\n";
        dllSetupDiagnostics += "  - Windows System32 directory (wrong version)\n";
        dllSetupDiagnostics += "Instead of from the application directory.\n\n";
        
        dllSetupDiagnostics += "Common causes:\n";
        dllSetupDiagnostics += "  1. Another Qt installation in system PATH\n";
        dllSetupDiagnostics += "  2. Insufficient permissions\n";
        dllSetupDiagnostics += "  3. PATH environment variable too large\n";
        dllSetupDiagnostics += "  4. Security software blocking DLL manipulation\n\n";
        
        dllSetupDiagnostics += "Likely errors if you continue:\n";
        dllSetupDiagnostics += "  - QTableView::dropEvent not found\n";
        dllSetupDiagnostics += "  - QWidget virtual function errors\n";
        dllSetupDiagnostics += "  - Qt plugin loading failures\n";
        dllSetupDiagnostics += "  - Application crash during UI initialization\n\n";
    }
    
    // Set Qt plugin path to application directory to prevent loading plugins from wrong Qt version
    // This must be done BEFORE creating QApplication
    if (pathLen > 0 && pathLen < MAX_PATH * 2) {
        std::wstring exePathStr(exePath);
        size_t lastSlash = exePathStr.find_last_of(L"\\/");
        std::wstring exeDirW = (lastSlash != std::wstring::npos) ? exePathStr.substr(0, lastSlash) : exePathStr;
        
        if (!exeDirW.empty()) {
            // Set QT_PLUGIN_PATH to ensure Qt plugins come from application directory
            std::wstring pluginPath = exeDirW + L"\\plugins";
            if (!SetEnvironmentVariableW(L"QT_PLUGIN_PATH", pluginPath.c_str())) {
                DWORD error = GetLastError();
                std::wstring errorMsg = L"Failed to set QT_PLUGIN_PATH environment variable (error ";
                errorMsg += std::to_wstring(error);
                errorMsg += L"). Qt may load plugins from wrong location.";
                MessageBoxW(NULL, errorMsg.c_str(), L"Qt Plugin Path Warning", MB_ICONWARNING | MB_OK);
            }
            
            // Also set QT_QPA_PLATFORM_PLUGIN_PATH for platform plugins
            if (!SetEnvironmentVariableW(L"QT_QPA_PLATFORM_PLUGIN_PATH", pluginPath.c_str())) {
                DWORD error = GetLastError();
                std::wstring errorMsg = L"Failed to set QT_QPA_PLATFORM_PLUGIN_PATH environment variable (error ";
                errorMsg += std::to_wstring(error);
                errorMsg += L"). Qt platform plugins may fail to load.";
                MessageBoxW(NULL, errorMsg.c_str(), L"Qt Platform Plugin Path Warning", MB_ICONWARNING | MB_OK);
            }
        }
    }
    // If GetModuleFileNameW failed or path was truncated, continue anyway
    // Qt will get the path later
    
    // NOW it's safe to create QApplication
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    const QString exeDir = QCoreApplication::applicationDirPath();
    QDir::setCurrent(exeDir);
    
    // Show detailed DLL setup warning if setup failed (now that QMessageBox is available)
    if (needsDllSetupWarning) {
        dllSetupDiagnostics += getPathDiagnostics();
        
        dllSetupDiagnostics += "\nRecommended actions:\n";
        dllSetupDiagnostics += "  1. Run as Administrator (allows DLL path manipulation)\n";
        dllSetupDiagnostics += "  2. Remove other Qt installations from system PATH\n";
        dllSetupDiagnostics += "  3. Check for conflicting Qt in C:\\Windows\\System32\n";
        dllSetupDiagnostics += "  4. Disable antivirus temporarily to test\n";
        dllSetupDiagnostics += "  5. Reinstall application to a simpler path (no spaces/special chars)\n\n";
        
        dllSetupDiagnostics += "You can copy this entire message for troubleshooting.\n";
        dllSetupDiagnostics += "Click 'Show Details >>' button below to see PATH diagnostics.\n\n";
        dllSetupDiagnostics += "=== For GitHub Copilot Users ===\n";
        dllSetupDiagnostics += "When errors occur, look for COPILOT_ERROR_*.md files in your logs directory.\n";
        dllSetupDiagnostics += "Copy the file contents and paste into Copilot Chat for step-by-step help.";
        
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle(QObject::tr("Critical DLL Setup Error"));
        msgBox.setText(QObject::tr(
            "Failed to configure DLL search paths properly.\n\n"
            "This WILL cause \"entry point not found\" errors when UI widgets are created.\n\n"
            "The application will likely crash during startup.\n\n"
            "Click 'Show Details' to see full diagnostic information (copyable).\n\n"
            "📋 Copilot Users: Error reports are saved to logs/COPILOT_ERROR_*.md\n"
            "   Copy that file and paste into Copilot Chat for help!"));
        msgBox.setDetailedText(dllSetupDiagnostics);
        msgBox.setStandardButtons(QMessageBox::Abort | QMessageBox::Ignore);
        msgBox.setDefaultButton(QMessageBox::Abort);
        
        // Make text copyable
        msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        
        // Find the QTextEdit in the message box and make it copyable
        QList<QTextEdit*> textEdits = msgBox.findChildren<QTextEdit*>();
        for (QTextEdit* edit : textEdits) {
            edit->setReadOnly(true);
            edit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        }
        
        int result = msgBox.exec();
        if (result == QMessageBox::Abort) {
            qCritical() << "User aborted due to DLL setup failure";
            return EXIT_FAILURE;
        }
        // User clicked Ignore - log this but we'll check Qt version below
        qWarning() << "User chose to ignore DLL setup failure - checking Qt version compatibility";
    }
    
    // Log DLL search setup status
    if (!secureSearchEnabled) {
        qWarning() << "SetDefaultDllDirectories unavailable; application directory prepended to PATH for DLL resolution.";
    } else {
        qInfo() << "Secure DLL search enabled - application directory prioritized for DLL loading";
    }
    
    // Log the actual Qt version being used to help diagnose version mismatch issues
    qInfo() << "Qt Runtime Version:" << qVersion() << "| Compiled with:" << QT_VERSION_STR;
    
    // CRITICAL: Check for Qt version mismatch - this WILL cause crashes
    QString runtimeVersion = QString::fromLatin1(qVersion());
    QString compileVersion = QString::fromLatin1(QT_VERSION_STR);
    
    if (runtimeVersion != compileVersion) {
        qCritical() << "CRITICAL: Qt version mismatch detected!";
        qCritical() << "Built with Qt" << QT_VERSION_STR << "but running with Qt" << qVersion();
        
        // Extract major version numbers
        QStringList runtimeParts = runtimeVersion.split('.');
        QStringList compileParts = compileVersion.split('.');
        
        bool majorVersionMismatch = false;
        if (runtimeParts.size() >= 1 && compileParts.size() >= 1) {
            int runtimeMajor = runtimeParts[0].toInt();
            int compileMajor = compileParts[0].toInt();
            majorVersionMismatch = (runtimeMajor != compileMajor);
        }
        
        // If DLL setup failed AND we have a major version mismatch, abort immediately
        // This prevents the dropEvent crash
        if (needsDllSetupWarning && majorVersionMismatch) {
            QMessageBox::critical(nullptr,
                QObject::tr("Fatal Qt Version Mismatch"),
                QObject::tr(
                    "Cannot continue: Qt major version mismatch detected.\n\n"
                    "Application was built with Qt %1\n"
                    "But system is loading Qt %2\n\n"
                    "This WILL cause crashes with errors like:\n"
                    "  - QTableView::dropEvent not found\n"
                    "  - QWidget virtual function errors\n\n"
                    "The application cannot run safely and will now exit.\n\n"
                    "To fix this issue:\n"
                    "1. Run as Administrator to allow proper DLL loading\n"
                    "2. Remove other Qt installations from system PATH\n"
                    "3. Reinstall this application")
                    .arg(compileVersion)
                    .arg(runtimeVersion));
            
            qCritical() << "ABORTING: Cannot run with Qt major version mismatch and failed DLL setup";
            return EXIT_FAILURE;
        }
        
        // If only minor version mismatch, warn but allow continuing
        if (!majorVersionMismatch) {
            qWarning() << "WARNING: Qt minor version mismatch detected - may cause issues";
        }
    }

    QString detectedCudaRuntime;
    const bool cudaRuntimeDetected = isCudaAvailable(&detectedCudaRuntime);
    const bool hasNvidiaDriver = isNvidiaDriverAvailable();

    BackendOverrides overrides = parse_backend_overrides(argc, argv);
    log_observed_arguments(overrides.observedArgs);
    if (!validate_override_conflict(overrides)) {
        return EXIT_FAILURE;
    }

    BackendAvailability availability = detect_backend_availability(exeDir,
                                                                   hasNvidiaDriver,
                                                                   cudaRuntimeDetected,
                                                                   detectedCudaRuntime);
    apply_override_flags(overrides, availability);
    if (maybe_prompt_cuda_download(overrides, availability)) {
        return EXIT_SUCCESS;
    }
    BackendSelection selection = resolve_backend_selection(overrides, availability);

    QString ggmlVariant = ggml_variant_for_selection(selection);
    QString ggmlPath = resolve_ggml_directory(exeDir, ggmlVariant, /*showError=*/false);
    if (ggmlPath.isEmpty()) {
        qWarning().noquote()
            << "Backend runtime directory missing for selection" << ggmlVariant
            << "- attempting fallback.";

        // Track fallback attempts to prevent infinite loop when cycling through backends.
        // Maximum 2 transitions allowed: e.g., Vulkan -> CUDA -> CPU or CUDA -> Vulkan -> CPU
        int fallbackAttempts = 0;
        const int maxFallbackAttempts = 2;

        while (ggmlPath.isEmpty() && fallbackAttempts < maxFallbackAttempts) {
            fallbackAttempts++;
            
            BackendSelection fallbackSelection = BackendSelection::Cpu;
            if (selection == BackendSelection::Vulkan && availability.cudaAvailable) {
                fallbackSelection = BackendSelection::Cuda;
            } else if (selection == BackendSelection::Cuda && availability.vulkanAvailable) {
                fallbackSelection = BackendSelection::Vulkan;
            }

            if (fallbackSelection != selection) {
                qInfo().noquote()
                    << "Falling back to backend"
                    << backend_tag_for_selection(fallbackSelection)
                    << "due to missing runtime directory.";
                selection = fallbackSelection;
                ggmlVariant = ggml_variant_for_selection(selection);
            } else {
                qInfo().noquote() << "Falling back to CPU backend.";
                selection = BackendSelection::Cpu;
                ggmlVariant = ggml_variant_for_selection(selection);
            }

            ggmlPath = resolve_ggml_directory(exeDir, ggmlVariant, /*showError=*/false);
        }

        if (ggmlPath.isEmpty()) {
            // Final attempt with error message
            ggmlPath = resolve_ggml_directory(exeDir, ggmlVariant, /*showError=*/true);
            if (ggmlPath.isEmpty()) {
                return EXIT_FAILURE;
            }
        }
    }

    log_runtime_availability(availability, selection);

    const bool useCuda = (selection == BackendSelection::Cuda);
    const bool useVulkan = (selection == BackendSelection::Vulkan);
    configure_runtime_paths(exeDir, ggmlPath, secureSearchEnabled, useCuda, useVulkan);

    // NEW: Check DLL compatibility before launching
    if (!check_dll_compatibility(ggmlPath, exeDir)) {
        qInfo() << "User aborted due to DLL version mismatch";
        return EXIT_FAILURE;
    }

    bool console_log_flag = false;
    QStringList forwardedArgs = build_forwarded_args(argc, argv, console_log_flag);
    if (console_log_flag) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            // Redirect stdout, stderr, and stdin to the parent console.
            // Check return values to ensure redirection succeeds and avoid silent failures.
            FILE* f = nullptr;
            if (freopen_s(&f, "CONOUT$", "w", stdout) != 0 || f == nullptr) {
                qWarning() << "Failed to redirect stdout to console";
            }
            f = nullptr;
            if (freopen_s(&f, "CONOUT$", "w", stderr) != 0 || f == nullptr) {
                qWarning() << "Failed to redirect stderr to console";
            }
            f = nullptr;
            if (freopen_s(&f, "CONIN$", "r", stdin) != 0 || f == nullptr) {
                qWarning() << "Failed to redirect stdin from console";
            }
        } else {
            qWarning() << "Failed to attach to parent console";
        }
    }

    const QString mainExecutable = resolveExecutableName(exeDir);
    if (mainExecutable.isEmpty()) {
        // resolveExecutableName already logged the error
        QMessageBox::critical(nullptr,
            QObject::tr("Missing Executable"),
            QObject::tr("The main application executable (aifilesorter.exe) was not found.\n\n"
                       "Installation directory: %1\n\n"
                       "Please reinstall the application or verify the installation is complete.")
                .arg(QDir::toNativeSeparators(exeDir)));
        return EXIT_FAILURE;
    }
    
    if (!launch_main_process(mainExecutable, forwardedArgs, selection, ggmlPath)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
