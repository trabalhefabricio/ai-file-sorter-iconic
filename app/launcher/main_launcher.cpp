/**
 * AI File Sorter Launcher
 * 
 * A simple launcher that allows users to choose between the GUI and TUI versions
 * of AI File Sorter.
 * 
 * Usage:
 *   aifilesorter-launcher [options]
 * 
 * Options:
 *   --gui         Launch GUI version directly
 *   --tui         Launch TUI version directly
 *   --help        Show help message
 *   --version     Show version information
 * 
 * Without arguments, presents an interactive menu to choose the interface.
 */

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace {

constexpr const char* VERSION = "1.0.0";
constexpr const char* APP_NAME = "AI File Sorter Launcher";

// Executable names
#ifdef _WIN32
constexpr const char* GUI_EXECUTABLE = "aifilesorter.exe";
constexpr const char* TUI_EXECUTABLE = "aifilesorter-tui.exe";
#else
constexpr const char* GUI_EXECUTABLE = "aifilesorter";
constexpr const char* TUI_EXECUTABLE = "aifilesorter-tui";
#endif

void print_version() {
    std::cout << APP_NAME << " v" << VERSION << std::endl;
    std::cout << "Choose between GUI and TUI interfaces for AI File Sorter" << std::endl;
}

void print_help() {
    std::cout << APP_NAME << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: aifilesorter-launcher [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --gui         Launch the graphical user interface directly" << std::endl;
    std::cout << "  --tui         Launch the terminal user interface directly" << std::endl;
    std::cout << "  --help        Show this help message" << std::endl;
    std::cout << "  --version     Show version information" << std::endl;
    std::cout << std::endl;
    std::cout << "Without arguments, an interactive menu is displayed to choose" << std::endl;
    std::cout << "between the available interfaces." << std::endl;
    std::cout << std::endl;
    std::cout << "Environment Variables:" << std::endl;
    std::cout << "  AI_FILE_SORTER_DEFAULT_UI   Set to 'gui' or 'tui' to skip the menu" << std::endl;
}

std::filesystem::path get_executable_directory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
#else
    char buffer[4096];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
    // Fallback: use current directory
    return std::filesystem::current_path();
#endif
}

std::filesystem::path find_executable(const char* name) {
    auto exe_dir = get_executable_directory();
    
    // Check in same directory as launcher
    auto same_dir = exe_dir / name;
    if (std::filesystem::exists(same_dir)) {
        return same_dir;
    }
    
    // Check in parent directory
    auto parent_dir = exe_dir.parent_path() / name;
    if (std::filesystem::exists(parent_dir)) {
        return parent_dir;
    }
    
    // Check in bin subdirectory
    auto bin_dir = exe_dir / "bin" / name;
    if (std::filesystem::exists(bin_dir)) {
        return bin_dir;
    }
    
    // Check on PATH
    if (const char* path_env = std::getenv("PATH")) {
        std::string path_str(path_env);
#ifdef _WIN32
        char delimiter = ';';
#else
        char delimiter = ':';
#endif
        size_t start = 0;
        size_t end = path_str.find(delimiter);
        
        while (end != std::string::npos) {
            auto dir = std::filesystem::path(path_str.substr(start, end - start)) / name;
            if (std::filesystem::exists(dir)) {
                return dir;
            }
            start = end + 1;
            end = path_str.find(delimiter, start);
        }
        
        // Check last segment
        auto dir = std::filesystem::path(path_str.substr(start)) / name;
        if (std::filesystem::exists(dir)) {
            return dir;
        }
    }
    
    return {};
}

bool is_available(const char* exe_name) {
    return !find_executable(exe_name).empty();
}

int launch_executable(const std::filesystem::path& exe_path, const std::vector<std::string>& args) {
    if (exe_path.empty()) {
        std::cerr << "Error: Executable not found" << std::endl;
        return 1;
    }
    
    std::string exe_str = exe_path.string();
    
#ifdef _WIN32
    // Build command line
    std::string cmd = "\"" + exe_str + "\"";
    for (const auto& arg : args) {
        cmd += " \"" + arg + "\"";
    }
    
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    
    // Create process
    if (!CreateProcessA(
        nullptr,
        cmd.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    )) {
        std::cerr << "Error: Failed to launch " << exe_str << std::endl;
        return 1;
    }
    
    // Wait for process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return static_cast<int>(exit_code);
#else
    // Fork and exec
    pid_t pid = fork();
    
    if (pid < 0) {
        std::cerr << "Error: Failed to fork process" << std::endl;
        return 1;
    }
    
    if (pid == 0) {
        // Child process
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(exe_str.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        execv(exe_str.c_str(), argv.data());
        
        // If execv returns, it failed
        std::cerr << "Error: Failed to execute " << exe_str << std::endl;
        _exit(1);
    }
    
    // Parent process: wait for child
    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    
    return 1;
#endif
}

int launch_gui(const std::vector<std::string>& args = {}) {
    auto path = find_executable(GUI_EXECUTABLE);
    if (path.empty()) {
        std::cerr << "Error: GUI version (" << GUI_EXECUTABLE << ") not found." << std::endl;
        std::cerr << "Please ensure AI File Sorter GUI is installed." << std::endl;
        return 1;
    }
    
    std::cout << "Launching GUI version..." << std::endl;
    return launch_executable(path, args);
}

int launch_tui(const std::vector<std::string>& args = {}) {
    auto path = find_executable(TUI_EXECUTABLE);
    if (path.empty()) {
        std::cerr << "Error: TUI version (" << TUI_EXECUTABLE << ") not found." << std::endl;
        std::cerr << "Please ensure AI File Sorter TUI is installed." << std::endl;
        return 1;
    }
    
    std::cout << "Launching TUI version..." << std::endl;
    return launch_executable(path, args);
}

void print_menu(bool gui_available, bool tui_available) {
    std::cout << std::endl;
    std::cout << "╔════════════════════════════════════════════╗" << std::endl;
    std::cout << "║       AI File Sorter - Launcher            ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════╣" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    
    if (gui_available) {
        std::cout << "║  [1] Graphical Interface (GUI)             ║" << std::endl;
        std::cout << "║      Full-featured desktop application     ║" << std::endl;
    } else {
        std::cout << "║  [1] Graphical Interface (NOT AVAILABLE)   ║" << std::endl;
    }
    
    std::cout << "║                                            ║" << std::endl;
    
    if (tui_available) {
        std::cout << "║  [2] Terminal Interface (TUI)              ║" << std::endl;
        std::cout << "║      Lightweight terminal-based version    ║" << std::endl;
    } else {
        std::cout << "║  [2] Terminal Interface (NOT AVAILABLE)    ║" << std::endl;
    }
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║  [Q] Quit                                  ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    std::cout << "Enter your choice (1/2/Q): ";
}

int run_interactive_menu() {
    bool gui_available = is_available(GUI_EXECUTABLE);
    bool tui_available = is_available(TUI_EXECUTABLE);
    
    if (!gui_available && !tui_available) {
        std::cerr << "Error: Neither GUI nor TUI version is available." << std::endl;
        std::cerr << "Please ensure at least one version of AI File Sorter is installed." << std::endl;
        return 1;
    }
    
    // Check for default UI preference
    if (const char* default_ui = std::getenv("AI_FILE_SORTER_DEFAULT_UI")) {
        std::string pref(default_ui);
        if (pref == "gui" && gui_available) {
            return launch_gui();
        } else if (pref == "tui" && tui_available) {
            return launch_tui();
        }
    }
    
    print_menu(gui_available, tui_available);
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice.empty()) {
        std::cout << "No selection made. Exiting." << std::endl;
        return 0;
    }
    
    char c = std::tolower(choice[0]);
    
    switch (c) {
        case '1':
            if (gui_available) {
                return launch_gui();
            } else {
                std::cerr << "GUI version is not available." << std::endl;
                return 1;
            }
            
        case '2':
            if (tui_available) {
                return launch_tui();
            } else {
                std::cerr << "TUI version is not available." << std::endl;
                return 1;
            }
            
        case 'q':
            std::cout << "Goodbye!" << std::endl;
            return 0;
            
        default:
            std::cerr << "Invalid choice: " << choice << std::endl;
            return 1;
    }
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::vector<std::string> forward_args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        
        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        }
        
        if (arg == "--version" || arg == "-v") {
            print_version();
            return 0;
        }
        
        if (arg == "--gui") {
            // Collect remaining arguments to forward
            for (int j = i + 1; j < argc; ++j) {
                forward_args.push_back(argv[j]);
            }
            return launch_gui(forward_args);
        }
        
        if (arg == "--tui") {
            // Collect remaining arguments to forward
            for (int j = i + 1; j < argc; ++j) {
                forward_args.push_back(argv[j]);
            }
            return launch_tui(forward_args);
        }
        
        // Unknown argument
        std::cerr << "Unknown option: " << arg << std::endl;
        std::cerr << "Use --help for usage information." << std::endl;
        return 1;
    }
    
    // No arguments: run interactive menu
    return run_interactive_menu();
}
