#ifndef TUI_APP_HPP
#define TUI_APP_HPP

#include "TuiSettings.hpp"
#include "DatabaseManager.hpp"
#include "FileScanner.hpp"
#include "Types.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <functional>

class TuiApp {
public:
    TuiApp();
    ~TuiApp();

    // Main entry point
    int run();

    // Version info
    static std::string get_version();

private:
    // Screen and components
    ftxui::ScreenInteractive screen_;
    TuiSettings settings_;
    std::unique_ptr<DatabaseManager> db_manager_;
    FileScanner file_scanner_;

    // State
    std::string current_path_;
    std::vector<FileEntry> scanned_files_;
    std::vector<CategorizedFile> categorized_files_;
    std::atomic<bool> stop_flag_{false};
    bool is_analyzing_{false};
    std::string status_message_;
    std::string progress_message_;
    int selected_menu_item_{0};
    int selected_file_index_{0};

    // UI State
    bool show_llm_selection_{false};
    bool show_settings_{false};
    bool show_results_{false};
    bool show_file_tinder_{false};
    bool show_whitelist_manager_{false};
    bool show_help_{false};

    // Settings state
    bool categorize_files_{true};
    bool categorize_directories_{false};
    bool use_subcategories_{true};
    bool use_consistency_hints_{false};
    bool use_whitelist_{false};

    // Build UI components
    ftxui::Component build_main_menu();
    ftxui::Component build_path_input();
    ftxui::Component build_options_panel();
    ftxui::Component build_file_list();
    ftxui::Component build_status_bar();
    ftxui::Component build_main_ui();

    // Dialog components
    ftxui::Component build_llm_selection_dialog();
    ftxui::Component build_settings_dialog();
    ftxui::Component build_results_dialog();
    ftxui::Component build_file_tinder_dialog();
    ftxui::Component build_whitelist_dialog();
    ftxui::Component build_help_dialog();

    // Actions
    void scan_directory();
    void analyze_files();
    void show_results();
    void execute_sort();
    void open_settings();
    void open_llm_selection();
    void open_file_tinder();
    void open_whitelist_manager();
    void show_about();
    void quit();

    // Helpers
    void update_status(const std::string& message);
    void load_settings();
    void save_settings();
    std::string llm_choice_to_display_string(LLMChoice choice) const;
    bool validate_path(const std::string& path) const;
};

#endif // TUI_APP_HPP
