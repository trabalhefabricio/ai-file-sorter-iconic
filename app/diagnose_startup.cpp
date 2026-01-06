/**
 * @file diagnose_startup.cpp
 * @brief Standalone diagnostic tool to identify startup issues
 * 
 * This tool can be run independently to diagnose why the application won't start.
 * It checks for common issues like missing DLLs, Qt version conflicts, and path problems.
 * 
 * Compile: cl diagnose_startup.cpp /EHsc
 * Run: diagnose_startup.exe
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <filesystem>
#include <dlfcn.h>
#include <unistd.h>
namespace fs = std::filesystem;
#endif

struct DiagnosticResult {
    bool success;
    std::string message;
    std::string details;
};

class StartupDiagnostics {
public:
    void run_all_checks() {
        std::cout << "=== AI File Sorter Startup Diagnostics ===" << std::endl;
        std::cout << "Generated: " << get_timestamp() << std::endl << std::endl;
        
        check_executable_location();
        check_dll_dependencies();
        check_qt_environment();
        check_ggml_directories();
        check_path_conflicts();
        check_permissions();
        
        std::cout << "\n=== Diagnostic Summary ===" << std::endl;
        print_recommendations();
        
        save_diagnostic_report();
    }

private:
    std::vector<DiagnosticResult> results;
    std::string exe_dir;
    
    std::string get_timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[100];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return buf;
    }
    
    std::string get_exe_directory() {
#ifdef _WIN32
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring ws(path);
        fs::path p(ws);
        return p.parent_path().string();
#else
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        fs::path p(std::string(result, (count > 0) ? count : 0));
        return p.parent_path().string();
#endif
    }
    
    void check_executable_location() {
        std::cout << "[1/6] Checking executable location..." << std::endl;
        
        exe_dir = get_exe_directory();
        std::cout << "  Application directory: " << exe_dir << std::endl;
        
        // Check if main executable exists
        fs::path main_exe = fs::path(exe_dir) / "aifilesorter.exe";
        fs::path starter_exe = fs::path(exe_dir) / "StartAiFileSorter.exe";
        
        bool has_main = fs::exists(main_exe);
        bool has_starter = fs::exists(starter_exe);
        
        std::cout << "  aifilesorter.exe: " << (has_main ? "✓ Found" : "✗ Missing") << std::endl;
        std::cout << "  StartAiFileSorter.exe: " << (has_starter ? "✓ Found" : "✗ Missing") << std::endl;
        
        if (!has_main) {
            results.push_back({false, "Main executable missing", 
                "aifilesorter.exe not found. Installation may be incomplete."});
        }
        if (!has_starter) {
            results.push_back({false, "Starter executable missing",
                "StartAiFileSorter.exe not found. You should always use this to launch the app."});
        }
        
        std::cout << std::endl;
    }
    
    void check_dll_dependencies() {
        std::cout << "[2/6] Checking DLL dependencies..." << std::endl;
        
#ifdef _WIN32
        // Check for critical Qt DLLs
        std::vector<std::string> critical_dlls = {
            "Qt6Core.dll",
            "Qt6Gui.dll", 
            "Qt6Widgets.dll",
            "llama.dll",
            "ggml.dll",
            "ggml-base.dll",
            "ggml-cpu.dll"
        };
        
        for (const auto& dll : critical_dlls) {
            fs::path dll_path = fs::path(exe_dir) / dll;
            bool exists = fs::exists(dll_path);
            std::cout << "  " << dll << ": " << (exists ? "✓" : "✗") << std::endl;
            
            if (!exists) {
                results.push_back({false, "Missing DLL: " + dll,
                    "Critical DLL not found in application directory."});
            }
        }
#endif
        
        std::cout << std::endl;
    }
    
    void check_qt_environment() {
        std::cout << "[3/6] Checking Qt environment..." << std::endl;
        
#ifdef _WIN32
        // Check PATH for conflicting Qt installations
        wchar_t path_buf[32768];
        DWORD size = GetEnvironmentVariableW(L"PATH", path_buf, 32768);
        
        if (size > 0) {
            std::wstring path_wstr(path_buf);
            std::string path_str(path_wstr.begin(), path_wstr.end());
            
            // Look for Qt in PATH
            bool found_qt = false;
            std::string qt_locations;
            
            size_t pos = 0;
            while ((pos = path_str.find(';', pos)) != std::string::npos) {
                size_t start = (pos == 0) ? 0 : path_str.rfind(';', pos - 1) + 1;
                std::string dir = path_str.substr(start, pos - start);
                
                if (dir.find("Qt") != std::string::npos || dir.find("qt") != std::string::npos) {
                    found_qt = true;
                    qt_locations += "    - " + dir + "\n";
                }
                
                pos++;
            }
            
            if (found_qt) {
                std::cout << "  ⚠ Warning: Qt found in system PATH:" << std::endl;
                std::cout << qt_locations;
                results.push_back({false, "Qt in system PATH",
                    "Other Qt installations in PATH may cause version conflicts."});
            } else {
                std::cout << "  ✓ No Qt installations found in system PATH" << std::endl;
            }
        }
#endif
        
        std::cout << std::endl;
    }
    
    void check_ggml_directories() {
        std::cout << "[4/6] Checking GGML runtime directories..." << std::endl;
        
        // Check for GGML backend directories
        fs::path ggml_base = fs::path(exe_dir) / "lib" / "ggml";
        
        std::vector<std::string> backends = {"wocuda", "wcuda", "wvulkan"};
        
        for (const auto& backend : backends) {
            fs::path backend_dir = ggml_base / backend;
            bool exists = fs::exists(backend_dir);
            std::cout << "  " << backend << ": " << (exists ? "✓" : "✗") << std::endl;
            
            if (!exists) {
                results.push_back({false, "Missing backend: " + backend,
                    "GGML backend directory not found. Build may be incomplete."});
            }
        }
        
        std::cout << std::endl;
    }
    
    void check_path_conflicts() {
        std::cout << "[5/6] Checking for path conflicts..." << std::endl;
        
        // Check if path has spaces or special characters
        if (exe_dir.find(' ') != std::string::npos) {
            std::cout << "  ⚠ Path contains spaces (may cause issues)" << std::endl;
            results.push_back({false, "Path has spaces",
                "Installation path contains spaces which may cause DLL loading issues."});
        } else {
            std::cout << "  ✓ Path has no spaces" << std::endl;
        }
        
        // Check path length
        if (exe_dir.length() > 200) {
            std::cout << "  ⚠ Path is very long (" << exe_dir.length() << " chars)" << std::endl;
            results.push_back({false, "Path too long",
                "Very long paths can cause issues on Windows."});
        } else {
            std::cout << "  ✓ Path length OK" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    void check_permissions() {
        std::cout << "[6/6] Checking file permissions..." << std::endl;
        
        // Try to create a test file
        fs::path test_file = fs::path(exe_dir) / "test_write.tmp";
        
        std::ofstream ofs(test_file);
        if (ofs.is_open()) {
            ofs << "test";
            ofs.close();
            fs::remove(test_file);
            std::cout << "  ✓ Write permissions OK" << std::endl;
        } else {
            std::cout << "  ✗ Cannot write to application directory" << std::endl;
            results.push_back({false, "No write permissions",
                "Cannot write to application directory. May need administrator rights."});
        }
        
        std::cout << std::endl;
    }
    
    void print_recommendations() {
        if (results.empty()) {
            std::cout << "✓ All checks passed! Application should start normally." << std::endl;
            return;
        }
        
        std::cout << "Found " << results.size() << " issue(s):\n" << std::endl;
        
        int issue_num = 1;
        for (const auto& result : results) {
            std::cout << issue_num++ << ". " << result.message << std::endl;
            std::cout << "   " << result.details << std::endl << std::endl;
        }
        
        std::cout << "Recommended Actions:" << std::endl;
        std::cout << "1. Use StartAiFileSorter.exe instead of aifilesorter.exe" << std::endl;
        std::cout << "2. Remove other Qt installations from system PATH" << std::endl;
        std::cout << "3. Run as Administrator if permission issues detected" << std::endl;
        std::cout << "4. Reinstall if critical DLLs are missing" << std::endl;
        std::cout << "5. Install to a path without spaces (e.g., C:\\AIFileSorter)" << std::endl;
    }
    
    void save_diagnostic_report() {
        fs::path report_path = fs::path(exe_dir) / "startup_diagnostic.txt";
        
        std::ofstream report(report_path);
        if (!report.is_open()) {
            std::cout << "\n⚠ Could not save diagnostic report to disk" << std::endl;
            return;
        }
        
        report << "=== AI File Sorter Startup Diagnostic Report ===" << std::endl;
        report << "Generated: " << get_timestamp() << std::endl;
        report << "Directory: " << exe_dir << std::endl << std::endl;
        
        report << "Issues found: " << results.size() << std::endl << std::endl;
        
        for (const auto& result : results) {
            report << "- " << result.message << std::endl;
            report << "  " << result.details << std::endl << std::endl;
        }
        
        report.close();
        
        std::cout << "\n📄 Full report saved to: " << report_path << std::endl;
    }
};

int main() {
    StartupDiagnostics diag;
    diag.run_all_checks();
    
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
}
