#ifndef DLL_VERSION_CHECKER_HPP
#define DLL_VERSION_CHECKER_HPP

#include <QString>
#include <QStringList>
#include <memory>

/**
 * @brief Checks DLL versions and exports to detect version mismatches
 * 
 * This class helps prevent "entry point not found" errors by verifying
 * that required DLL exports exist before attempting to load them.
 */
class DllVersionChecker {
public:
    struct CheckResult {
        bool isCompatible{false};
        QString errorMessage;
        QStringList missingSymbols;
        QString dllVersion;
    };

    /**
     * @brief Check if a DLL exports the required symbols
     * @param dllPath Full path to the DLL file
     * @param requiredSymbols List of symbol names that must be exported
     * @return CheckResult with compatibility status and details
     */
    static CheckResult checkDllExports(const QString& dllPath, 
                                       const QStringList& requiredSymbols);

    /**
     * @brief Check if llama.dll has all required exports for the current version
     * @param dllPath Path to llama.dll or ggml.dll
     * @return CheckResult with compatibility status
     */
    static CheckResult checkLlamaDllCompatibility(const QString& dllPath);

    /**
     * @brief Get list of critical GGML symbols required by the application
     * @return List of required symbol names
     */
    static QStringList getRequiredGgmlSymbols();

    /**
     * @brief Check Qt runtime version compatibility
     * @return CheckResult with compatibility status and version info
     */
    static CheckResult checkQtRuntimeCompatibility();

private:
    static bool isDumpbinAvailable();
    static QString findDumpbinPath();
    static QStringList extractExports(const QString& dumpbinOutput);
};

#endif // DLL_VERSION_CHECKER_HPP
