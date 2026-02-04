/**
 * AI File Sorter - TUI Edition
 * 
 * Text-based User Interface version of the AI File Sorter application.
 * This version removes Qt dependencies and uses FTXUI for terminal-based UI.
 * 
 * Usage:
 *   aifilesorter-tui [options]
 * 
 * Options:
 *   --version    Show version information
 *   --help       Show help message
 *   --path PATH  Set initial directory path
 */

#include "TuiApp.hpp"
#include "Logger.hpp"

#include <iostream>
#include <cstring>
#include <cstdlib>

#include <curl/curl.h>

namespace {

void print_version() {
    std::cout << TuiApp::get_version() << std::endl;
    std::cout << "A text-based file organizer powered by AI" << std::endl;
    std::cout << "https://github.com/hyperfield/ai-file-sorter" << std::endl;
}

void print_help() {
    std::cout << "AI File Sorter TUI - Text-based User Interface" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: aifilesorter-tui [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --version     Show version information and exit" << std::endl;
    std::cout << "  --help        Show this help message and exit" << std::endl;
    std::cout << "  --path PATH   Set initial directory path to sort" << std::endl;
    std::cout << std::endl;
    std::cout << "Keyboard shortcuts (in application):" << std::endl;
    std::cout << "  S             Scan directory for files" << std::endl;
    std::cout << "  A             Analyze files with AI" << std::endl;
    std::cout << "  R             View categorization results" << std::endl;
    std::cout << "  T             Open File Tinder (cleanup tool)" << std::endl;
    std::cout << "  L             Select LLM model" << std::endl;
    std::cout << "  W             Manage whitelists" << std::endl;
    std::cout << "  O             Open options/settings" << std::endl;
    std::cout << "  H/F1          Show help" << std::endl;
    std::cout << "  Q/ESC         Quit application" << std::endl;
    std::cout << std::endl;
    std::cout << "File Tinder shortcuts:" << std::endl;
    std::cout << "  Left Arrow    Keep file" << std::endl;
    std::cout << "  Right Arrow   Mark for deletion" << std::endl;
    std::cout << "  Down Arrow    Skip file" << std::endl;
    std::cout << "  U             Undo last decision" << std::endl;
    std::cout << std::endl;
}

bool initialize_loggers() {
    try {
        Logger::setup_loggers();
        return true;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Warning: Failed to initialize loggers: %s\n", e.what());
        return false;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string initial_path;
    
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        }
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        }
        if (std::strcmp(argv[i], "--path") == 0 && i + 1 < argc) {
            initial_path = argv[++i];
        }
    }
    
    // Initialize logging
    initialize_loggers();
    
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Run the TUI application
    int result = 0;
    try {
        TuiApp app;
        result = app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        result = 1;
    }
    
    // Cleanup
    curl_global_cleanup();
    
    return result;
}
