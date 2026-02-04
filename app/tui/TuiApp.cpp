#include "TuiApp.hpp"
#include "TuiLLMSelection.hpp"
#include "TuiCategorizationProgress.hpp"
#include "TuiCategorizationResults.hpp"
#include "TuiFileTinder.hpp"
#include "TuiWhitelistManager.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <filesystem>
#include <iostream>
#include <algorithm>

using namespace ftxui;

namespace {
constexpr const char* APP_VERSION = "1.4.5-tui";
constexpr const char* APP_NAME = "AI File Sorter TUI";
}

TuiApp::TuiApp()
    : screen_(ScreenInteractive::Fullscreen())
{
    load_settings();
    
    // Initialize database manager
    std::string db_path = settings_.get_config_dir() + "/categorization_cache.db";
    db_manager_ = std::make_unique<DatabaseManager>(db_path);
    
    // Set initial path
    current_path_ = settings_.get_sort_folder();
    if (current_path_.empty() || !std::filesystem::exists(current_path_)) {
        current_path_ = std::filesystem::current_path().string();
    }
    
    // Load options from settings
    categorize_files_ = settings_.get_categorize_files();
    categorize_directories_ = settings_.get_categorize_directories();
    use_subcategories_ = settings_.get_use_subcategories();
    use_consistency_hints_ = settings_.get_use_consistency_hints();
    use_whitelist_ = settings_.get_use_whitelist();
    
    update_status("Ready. Press F1 for help.");
}

TuiApp::~TuiApp() {
    save_settings();
}

std::string TuiApp::get_version() {
    return std::string(APP_NAME) + " v" + APP_VERSION;
}

void TuiApp::load_settings() {
    try {
        settings_.load();
    } catch (const std::exception& e) {
        status_message_ = std::string("Warning: Could not load settings: ") + e.what();
    }
}

void TuiApp::save_settings() {
    settings_.set_sort_folder(current_path_);
    settings_.set_categorize_files(categorize_files_);
    settings_.set_categorize_directories(categorize_directories_);
    settings_.set_use_subcategories(use_subcategories_);
    settings_.set_use_consistency_hints(use_consistency_hints_);
    settings_.set_use_whitelist(use_whitelist_);
    settings_.save();
}

void TuiApp::update_status(const std::string& message) {
    status_message_ = message;
}

bool TuiApp::validate_path(const std::string& path) const {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

std::string TuiApp::llm_choice_to_display_string(LLMChoice choice) const {
    switch (choice) {
        case LLMChoice::Local_3b: return "Local LLM (3B)";
        case LLMChoice::Local_7b: return "Local LLM (7B)";
        case LLMChoice::Remote_OpenAI: return "ChatGPT (OpenAI)";
        case LLMChoice::Remote_Gemini: return "Gemini (Google)";
        case LLMChoice::Custom: return "Custom LLM";
        default: return "Not Selected";
    }
}

Component TuiApp::build_main_menu() {
    std::vector<std::string> menu_entries = {
        " [S] Scan Directory ",
        " [A] Analyze Files  ",
        " [R] View Results   ",
        " [T] File Tinder    ",
        " [L] Select LLM     ",
        " [W] Whitelists     ",
        " [O] Options        ",
        " [H] Help           ",
        " [Q] Quit           ",
    };
    
    auto menu = Menu(&menu_entries, &selected_menu_item_);
    
    return menu | CatchEvent([this](Event event) {
        if (event == Event::Character('s') || event == Event::Character('S')) {
            scan_directory();
            return true;
        }
        if (event == Event::Character('a') || event == Event::Character('A')) {
            analyze_files();
            return true;
        }
        if (event == Event::Character('r') || event == Event::Character('R')) {
            show_results_ = true;
            return true;
        }
        if (event == Event::Character('t') || event == Event::Character('T')) {
            show_file_tinder_ = true;
            return true;
        }
        if (event == Event::Character('l') || event == Event::Character('L')) {
            show_llm_selection_ = true;
            return true;
        }
        if (event == Event::Character('w') || event == Event::Character('W')) {
            show_whitelist_manager_ = true;
            return true;
        }
        if (event == Event::Character('o') || event == Event::Character('O')) {
            show_settings_ = true;
            return true;
        }
        if (event == Event::Character('h') || event == Event::Character('H') || event == Event::F1) {
            show_help_ = true;
            return true;
        }
        if (event == Event::Character('q') || event == Event::Character('Q') || event == Event::Escape) {
            quit();
            return true;
        }
        return false;
    });
}

Component TuiApp::build_path_input() {
    auto input = Input(&current_path_, "Enter directory path...");
    
    return input | CatchEvent([this](Event event) {
        if (event == Event::Return) {
            if (validate_path(current_path_)) {
                scan_directory();
            } else {
                update_status("Error: Invalid directory path");
            }
            return true;
        }
        return false;
    });
}

Component TuiApp::build_options_panel() {
    auto checkbox_files = Checkbox("Categorize Files", &categorize_files_);
    auto checkbox_dirs = Checkbox("Categorize Directories", &categorize_directories_);
    auto checkbox_subcats = Checkbox("Use Subcategories", &use_subcategories_);
    auto checkbox_hints = Checkbox("Use Consistency Hints", &use_consistency_hints_);
    auto checkbox_whitelist = Checkbox("Use Whitelist", &use_whitelist_);
    
    return Container::Vertical({
        checkbox_files,
        checkbox_dirs,
        checkbox_subcats,
        checkbox_hints,
        checkbox_whitelist,
    });
}

Component TuiApp::build_file_list() {
    std::vector<std::string> file_entries;
    for (const auto& file : scanned_files_) {
        std::string prefix = (file.type == FileType::Directory) ? "[D] " : "[F] ";
        file_entries.push_back(prefix + file.file_name);
    }
    
    if (file_entries.empty()) {
        file_entries.push_back("(No files scanned yet)");
    }
    
    return Menu(&file_entries, &selected_file_index_);
}

Component TuiApp::build_status_bar() {
    return Renderer([this] {
        return hbox({
            text(" " + status_message_ + " ") | flex,
            separator(),
            text(" LLM: " + llm_choice_to_display_string(settings_.get_llm_choice()) + " "),
            separator(),
            text(" Files: " + std::to_string(scanned_files_.size()) + " "),
        }) | bgcolor(Color::Blue) | color(Color::White);
    });
}

Component TuiApp::build_help_dialog() {
    auto close_button = Button(" Close ", [this] { show_help_ = false; });
    
    auto content = Renderer([this] {
        return vbox({
            text("═══════════════════════════════════════") | bold | center,
            text("       AI FILE SORTER TUI - HELP       ") | bold | center,
            text("═══════════════════════════════════════") | bold | center,
            text(""),
            text("Keyboard Shortcuts:") | bold,
            text(""),
            hbox({text("  S  ") | bold, text("- Scan directory for files")}),
            hbox({text("  A  ") | bold, text("- Analyze files with AI")}),
            hbox({text("  R  ") | bold, text("- View categorization results")}),
            hbox({text("  T  ") | bold, text("- Open File Tinder (cleanup tool)")}),
            hbox({text("  L  ") | bold, text("- Select LLM model")}),
            hbox({text("  W  ") | bold, text("- Manage whitelists")}),
            hbox({text("  O  ") | bold, text("- Open options/settings")}),
            hbox({text("  H  ") | bold, text("- Show this help")}),
            hbox({text("  Q  ") | bold, text("- Quit application")}),
            text(""),
            hbox({text(" F1  ") | bold, text("- Show help")}),
            hbox({text(" ESC ") | bold, text("- Close dialogs / Quit")}),
            hbox({text(" Tab ") | bold, text("- Navigate between panels")}),
            hbox({text("Enter") | bold, text("- Confirm selection")}),
            text(""),
            text("═══════════════════════════════════════") | bold | center,
            text("   Press ESC or click Close to exit    ") | center,
        });
    });
    
    return Container::Vertical({
        content,
        close_button | center,
    }) | border | bgcolor(Color::Default);
}

Component TuiApp::build_llm_selection_dialog() {
    return TuiLLMSelection::Create(settings_, [this](bool accepted) {
        show_llm_selection_ = false;
        if (accepted) {
            save_settings();
            update_status("LLM selection updated");
        }
    });
}

Component TuiApp::build_settings_dialog() {
    auto close_button = Button(" Save & Close ", [this] {
        save_settings();
        show_settings_ = false;
        update_status("Settings saved");
    });
    
    auto cancel_button = Button(" Cancel ", [this] {
        show_settings_ = false;
    });
    
    auto content = Container::Vertical({
        Checkbox("Categorize Files", &categorize_files_),
        Checkbox("Categorize Directories", &categorize_directories_),
        Checkbox("Use Subcategories", &use_subcategories_),
        Checkbox("Use Consistency Hints", &use_consistency_hints_),
        Checkbox("Use Whitelist", &use_whitelist_),
    });
    
    auto buttons = Container::Horizontal({
        close_button,
        cancel_button,
    });
    
    return Container::Vertical({
        Renderer([] { return text(" Settings ") | bold | center; }),
        content,
        Renderer([] { return separator(); }),
        buttons | center,
    }) | border;
}

Component TuiApp::build_results_dialog() {
    return TuiCategorizationResults::Create(
        categorized_files_,
        *db_manager_,
        [this] {
            show_results_ = false;
        },
        [this](const std::vector<CategorizedFile>& files) {
            // Execute sort callback
            execute_sort();
        }
    );
}

Component TuiApp::build_file_tinder_dialog() {
    return TuiFileTinder::Create(
        current_path_,
        *db_manager_,
        [this] {
            show_file_tinder_ = false;
        }
    );
}

Component TuiApp::build_whitelist_dialog() {
    return TuiWhitelistManager::Create(
        settings_,
        [this] {
            show_whitelist_manager_ = false;
            save_settings();
        }
    );
}

Component TuiApp::build_main_ui() {
    // Header
    auto header = Renderer([] {
        return hbox({
            text("╔═══════════════════════════════════════════════════════════════════╗") | bold,
        });
    });
    
    auto title = Renderer([] {
        return hbox({
            text("║") | bold,
            text("              AI FILE SORTER - TUI Edition              ") | bold | center | flex,
            text("║") | bold,
        });
    });
    
    auto header_bottom = Renderer([] {
        return hbox({
            text("╚═══════════════════════════════════════════════════════════════════╝") | bold,
        });
    });
    
    // Main content
    auto path_section = Container::Vertical({
        Renderer([this] {
            return vbox({
                text(" Directory Path: ") | bold,
                text(" " + current_path_ + " ") | bgcolor(Color::GrayDark),
            });
        }),
        build_path_input(),
    });
    
    auto menu = build_main_menu();
    auto options = build_options_panel();
    
    // File list with dynamic content
    auto file_list = Renderer([this] {
        std::vector<Element> entries;
        entries.push_back(text(" Scanned Files (" + std::to_string(scanned_files_.size()) + "): ") | bold);
        entries.push_back(separator());
        
        if (scanned_files_.empty()) {
            entries.push_back(text("   (No files scanned yet)") | dim);
        } else {
            int display_count = std::min(static_cast<int>(scanned_files_.size()), 15);
            for (int i = 0; i < display_count; ++i) {
                const auto& file = scanned_files_[i];
                std::string prefix = (file.type == FileType::Directory) ? " [D] " : " [F] ";
                auto color = (file.type == FileType::Directory) ? Color::Yellow : Color::White;
                entries.push_back(text(prefix + file.file_name) | color(color));
            }
            if (scanned_files_.size() > 15) {
                entries.push_back(text("   ... and " + std::to_string(scanned_files_.size() - 15) + " more") | dim);
            }
        }
        
        return vbox(entries) | border | flex;
    });
    
    auto status_bar = build_status_bar();
    
    auto main_layout = Container::Vertical({
        Renderer([this] {
            return vbox({
                text("════════════════════════════════════════════════════════") | bold | center,
                text("         AI FILE SORTER - TUI Edition v" + std::string(APP_VERSION) + "         ") | bold | center,
                text("════════════════════════════════════════════════════════") | bold | center,
            });
        }),
        Container::Horizontal({
            Container::Vertical({
                Renderer([] { return text(" Menu ") | bold | center; }),
                menu,
            }) | border | size(WIDTH, EQUAL, 25),
            Container::Vertical({
                Renderer([this] {
                    return vbox({
                        text(" Directory: ") | bold,
                        text(" " + current_path_) | bgcolor(Color::GrayDark),
                    });
                }),
                Container::Horizontal({
                    Container::Vertical({
                        Renderer([] { return text(" Options ") | bold | center; }),
                        options,
                    }) | border | size(WIDTH, EQUAL, 30),
                    file_list | flex,
                }),
            }) | flex,
        }) | flex,
        status_bar,
    });
    
    // Add modal dialogs
    auto with_modals = main_layout;
    
    // LLM Selection dialog
    with_modals = Modal(with_modals, build_llm_selection_dialog(), &show_llm_selection_);
    
    // Settings dialog  
    with_modals = Modal(with_modals, build_settings_dialog(), &show_settings_);
    
    // Results dialog
    with_modals = Modal(with_modals, build_results_dialog(), &show_results_);
    
    // File Tinder dialog
    with_modals = Modal(with_modals, build_file_tinder_dialog(), &show_file_tinder_);
    
    // Whitelist dialog
    with_modals = Modal(with_modals, build_whitelist_dialog(), &show_whitelist_manager_);
    
    // Help dialog
    with_modals = Modal(with_modals, build_help_dialog(), &show_help_);
    
    return with_modals;
}

void TuiApp::scan_directory() {
    if (!validate_path(current_path_)) {
        update_status("Error: Invalid directory path - " + current_path_);
        return;
    }
    
    update_status("Scanning directory...");
    
    FileScanOptions options = FileScanOptions::None;
    if (categorize_files_) {
        options = options | FileScanOptions::Files;
    }
    if (categorize_directories_) {
        options = options | FileScanOptions::Directories;
    }
    
    try {
        scanned_files_ = file_scanner_.get_directory_entries(current_path_, options);
        update_status("Scanned " + std::to_string(scanned_files_.size()) + " items in " + current_path_);
    } catch (const std::exception& e) {
        update_status("Error scanning directory: " + std::string(e.what()));
    }
}

void TuiApp::analyze_files() {
    if (scanned_files_.empty()) {
        update_status("No files to analyze. Scan a directory first.");
        return;
    }
    
    if (!settings_.is_llm_chosen()) {
        update_status("Please select an LLM first (press L)");
        show_llm_selection_ = true;
        return;
    }
    
    update_status("Starting analysis...");
    
    // Create progress dialog and run analysis
    TuiCategorizationProgress progress(settings_, *db_manager_, scanned_files_, screen_);
    auto result = progress.run();
    
    if (result.success) {
        categorized_files_ = result.categorized_files;
        update_status("Analysis complete. " + std::to_string(categorized_files_.size()) + " files categorized.");
        show_results_ = true;
    } else {
        update_status("Analysis failed or cancelled: " + result.error_message);
    }
}

void TuiApp::show_results() {
    if (categorized_files_.empty()) {
        update_status("No categorization results. Run analysis first.");
        return;
    }
    show_results_ = true;
}

void TuiApp::execute_sort() {
    if (categorized_files_.empty()) {
        update_status("No files to sort");
        return;
    }
    
    int moved_count = 0;
    int error_count = 0;
    
    for (const auto& file : categorized_files_) {
        if (file.category.empty()) {
            continue;
        }
        
        try {
            std::filesystem::path source(file.file_path);
            std::filesystem::path target_dir = std::filesystem::path(current_path_) / file.category;
            
            if (!file.subcategory.empty()) {
                target_dir /= file.subcategory;
            }
            
            if (!std::filesystem::exists(target_dir)) {
                std::filesystem::create_directories(target_dir);
            }
            
            std::filesystem::path target = target_dir / file.file_name;
            
            if (source != target && std::filesystem::exists(source)) {
                std::filesystem::rename(source, target);
                ++moved_count;
            }
        } catch (const std::exception& e) {
            ++error_count;
        }
    }
    
    update_status("Sorted " + std::to_string(moved_count) + " files" + 
                  (error_count > 0 ? " (" + std::to_string(error_count) + " errors)" : ""));
    
    // Refresh file list
    scan_directory();
}

void TuiApp::open_settings() {
    show_settings_ = true;
}

void TuiApp::open_llm_selection() {
    show_llm_selection_ = true;
}

void TuiApp::open_file_tinder() {
    show_file_tinder_ = true;
}

void TuiApp::open_whitelist_manager() {
    show_whitelist_manager_ = true;
}

void TuiApp::show_about() {
    show_help_ = true;
}

void TuiApp::quit() {
    save_settings();
    screen_.Exit();
}

int TuiApp::run() {
    // Check if LLM is configured
    if (!settings_.is_llm_chosen()) {
        show_llm_selection_ = true;
    }
    
    auto main_ui = build_main_ui();
    screen_.Loop(main_ui);
    
    return 0;
}
