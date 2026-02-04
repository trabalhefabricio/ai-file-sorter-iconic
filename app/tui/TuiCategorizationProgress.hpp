#ifndef TUI_CATEGORIZATION_PROGRESS_HPP
#define TUI_CATEGORIZATION_PROGRESS_HPP

#include "TuiSettings.hpp"
#include "DatabaseManager.hpp"
#include "Types.hpp"

#include <ftxui/component/screen_interactive.hpp>
#include <vector>
#include <string>
#include <atomic>

/**
 * TUI component for showing categorization progress
 */
class TuiCategorizationProgress {
public:
    struct Result {
        bool success{false};
        std::vector<CategorizedFile> categorized_files;
        std::string error_message;
    };
    
    TuiCategorizationProgress(TuiSettings& settings,
                              DatabaseManager& db_manager,
                              const std::vector<FileEntry>& files,
                              ftxui::ScreenInteractive& parent_screen);
    
    Result run();
    void stop();
    
private:
    TuiSettings& settings_;
    DatabaseManager& db_manager_;
    std::vector<FileEntry> files_;
    ftxui::ScreenInteractive& parent_screen_;
    
    std::atomic<bool> stop_flag_{false};
    std::atomic<int> progress_{0};
    std::atomic<int> total_{0};
    std::string current_file_;
    std::string last_category_;
    std::vector<std::string> log_messages_;
    std::vector<CategorizedFile> results_;
    std::string error_message_;
    bool completed_{false};
    
    void run_categorization();
    void add_log_message(const std::string& message);
};

#endif // TUI_CATEGORIZATION_PROGRESS_HPP
