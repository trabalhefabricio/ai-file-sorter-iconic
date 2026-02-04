#include "TuiCategorizationProgress.hpp"
#include "CategorizationService.hpp"
#include "LocalLLMClient.hpp"
#include "LLMClient.hpp"
#include "GeminiClient.hpp"
#include "Logger.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <thread>
#include <chrono>
#include <mutex>
#include <filesystem>

using namespace ftxui;

TuiCategorizationProgress::TuiCategorizationProgress(
    TuiSettings& settings,
    DatabaseManager& db_manager,
    const std::vector<FileEntry>& files,
    ScreenInteractive& parent_screen)
    : settings_(settings)
    , db_manager_(db_manager)
    , files_(files)
    , parent_screen_(parent_screen)
{
    total_ = static_cast<int>(files.size());
}

void TuiCategorizationProgress::add_log_message(const std::string& message) {
    static std::mutex log_mutex;
    std::lock_guard<std::mutex> lock(log_mutex);
    log_messages_.push_back(message);
    if (log_messages_.size() > 10) {
        log_messages_.erase(log_messages_.begin());
    }
}

void TuiCategorizationProgress::stop() {
    stop_flag_ = true;
}

void TuiCategorizationProgress::run_categorization() {
    auto core_logger = Logger::get_logger("core_logger");
    
    // Create appropriate Settings object for CategorizationService
    // Note: This is a simplified adapter - in production, you'd want proper Settings
    
    try {
        add_log_message("Starting categorization...");
        
        bool is_local = (settings_.get_llm_choice() == LLMChoice::Local_3b ||
                        settings_.get_llm_choice() == LLMChoice::Local_7b ||
                        settings_.get_llm_choice() == LLMChoice::Custom);
        
        // Simple categorization loop for TUI
        for (size_t i = 0; i < files_.size() && !stop_flag_; ++i) {
            const auto& file = files_[i];
            current_file_ = file.file_name;
            progress_ = static_cast<int>(i + 1);
            
            add_log_message("Processing: " + file.file_name);
            
            // Try to get from cache first
            auto cached = db_manager_.get_categorization_from_db(file.file_name, file.type);
            
            CategorizedFile result;
            result.file_path = file.full_path;
            result.file_name = file.file_name;
            result.type = file.type;
            
            if (!cached.empty() && cached.size() >= 2 && !cached[0].empty()) {
                result.category = cached[0];
                result.subcategory = cached.size() > 1 ? cached[1] : "";
                result.from_cache = true;
                add_log_message("  -> (cached) " + result.category);
            } else {
                // For demo/testing, assign a basic category based on extension
                std::string ext;
                size_t dot_pos = file.file_name.rfind('.');
                if (dot_pos != std::string::npos) {
                    ext = file.file_name.substr(dot_pos + 1);
                }
                
                // Basic categorization rules
                if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp") {
                    result.category = "Images";
                } else if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "ogg") {
                    result.category = "Audio";
                } else if (ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "mov") {
                    result.category = "Video";
                } else if (ext == "doc" || ext == "docx" || ext == "pdf" || ext == "txt" || ext == "rtf") {
                    result.category = "Documents";
                } else if (ext == "zip" || ext == "rar" || ext == "7z" || ext == "tar" || ext == "gz") {
                    result.category = "Archives";
                } else if (ext == "exe" || ext == "msi" || ext == "dmg" || ext == "app") {
                    result.category = "Applications";
                } else if (ext == "cpp" || ext == "h" || ext == "py" || ext == "js" || ext == "java") {
                    result.category = "Source Code";
                } else if (file.type == FileType::Directory) {
                    result.category = "Folders";
                } else {
                    result.category = "Other";
                }
                
                add_log_message("  -> " + result.category);
                
                // Cache the result using resolve_category
                auto resolved = db_manager_.resolve_category(result.category, result.subcategory);
                std::string file_type_str = (file.type == FileType::Directory) ? "directory" : "file";
                db_manager_.insert_or_update_file_with_categorization(
                    file.file_name,
                    file_type_str,
                    std::filesystem::path(file.full_path).parent_path().string(),
                    resolved,
                    false);
            }
            
            last_category_ = result.category;
            results_.push_back(result);
            
            // Small delay to show progress
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        if (stop_flag_) {
            add_log_message("Categorization cancelled by user.");
            error_message_ = "Cancelled by user";
        } else {
            add_log_message("Categorization complete!");
        }
        
    } catch (const std::exception& e) {
        error_message_ = e.what();
        add_log_message("Error: " + error_message_);
    }
    
    completed_ = true;
}

TuiCategorizationProgress::Result TuiCategorizationProgress::run() {
    Result result;
    
    if (files_.empty()) {
        result.error_message = "No files to categorize";
        return result;
    }
    
    // Create a separate screen for the progress dialog
    auto screen = ScreenInteractive::Fullscreen();
    
    // Start categorization in background thread
    std::thread worker([this] {
        run_categorization();
    });
    
    // Build progress UI
    auto stop_button = Button(" Stop ", [this, &screen] {
        stop_flag_ = true;
        screen.Exit();
    });
    
    auto close_button = Button(" Close ", [&screen] {
        screen.Exit();
    });
    
    auto component = Renderer([this, &stop_button, &close_button] {
        float progress_pct = total_ > 0 ? static_cast<float>(progress_) / total_ : 0.0f;
        
        std::vector<Element> log_elements;
        for (const auto& msg : log_messages_) {
            log_elements.push_back(text(msg));
        }
        
        return vbox({
            text("════════════════════════════════════════") | bold | center,
            text("         CATEGORIZATION PROGRESS        ") | bold | center,
            text("════════════════════════════════════════") | bold | center,
            text(""),
            text(" Progress: " + std::to_string(progress_.load()) + " / " + std::to_string(total_.load())),
            gauge(progress_pct) | color(Color::Green),
            text(""),
            text(" Current: " + current_file_) | dim,
            text(" Category: " + last_category_) | color(Color::Yellow),
            text(""),
            separator(),
            text(" Log: ") | bold,
            vbox(log_elements) | size(HEIGHT, EQUAL, 10) | frame,
            separator(),
            text(""),
            hbox({
                completed_ ? close_button->Render() : stop_button->Render(),
            }) | center,
        }) | border | size(WIDTH, EQUAL, 60);
    });
    
    // Handle events
    auto with_events = CatchEvent(component, [this, &screen, &worker](Event event) {
        if (event == Event::Escape) {
            stop_flag_ = true;
            screen.Exit();
            return true;
        }
        
        // Check if completed and auto-close after brief delay
        if (completed_) {
            screen.Post([&screen] {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                screen.Exit();
            });
        }
        
        return false;
    });
    
    // Run the event loop
    screen.Loop(with_events);
    
    // Wait for worker thread
    if (worker.joinable()) {
        stop_flag_ = true;
        worker.join();
    }
    
    // Build result
    result.success = error_message_.empty() && !stop_flag_;
    result.categorized_files = results_;
    result.error_message = error_message_;
    
    return result;
}
