#include "TuiFileTinder.hpp"
#include "FileScanner.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <filesystem>
#include <algorithm>
#include <fstream>

using namespace ftxui;

namespace {

enum class TinderDecision {
    Pending,
    Keep,
    Delete,
    Skip
};

struct TinderState {
    std::string directory_path;
    DatabaseManager& db_manager;
    TuiFileTinder::CloseCallback on_close;
    
    std::vector<FileEntry> files;
    std::vector<TinderDecision> decisions;
    int current_index{0};
    std::string status_message;
    bool show_confirm_delete{false};
    int delete_count{0};
    int keep_count{0};
    int skip_count{0};
    
    TinderState(const std::string& path, DatabaseManager& db, TuiFileTinder::CloseCallback close_cb)
        : directory_path(path), db_manager(db), on_close(std::move(close_cb)) {
        scan_files();
    }
    
    void scan_files() {
        FileScanner scanner;
        files = scanner.get_directory_entries(directory_path, FileScanOptions::Files);
        decisions.resize(files.size(), TinderDecision::Pending);
        status_message = "Scanned " + std::to_string(files.size()) + " files";
    }
    
    void mark_keep() {
        if (current_index < static_cast<int>(files.size())) {
            if (decisions[current_index] != TinderDecision::Keep) {
                decisions[current_index] = TinderDecision::Keep;
                keep_count++;
            }
            move_next();
        }
    }
    
    void mark_delete() {
        if (current_index < static_cast<int>(files.size())) {
            if (decisions[current_index] != TinderDecision::Delete) {
                decisions[current_index] = TinderDecision::Delete;
                delete_count++;
            }
            move_next();
        }
    }
    
    void mark_skip() {
        if (current_index < static_cast<int>(files.size())) {
            if (decisions[current_index] != TinderDecision::Skip) {
                decisions[current_index] = TinderDecision::Skip;
                skip_count++;
            }
            move_next();
        }
    }
    
    void move_next() {
        if (current_index < static_cast<int>(files.size()) - 1) {
            current_index++;
        }
    }
    
    void move_prev() {
        if (current_index > 0) {
            current_index--;
        }
    }
    
    void undo_last() {
        if (current_index > 0) {
            current_index--;
            auto& decision = decisions[current_index];
            if (decision == TinderDecision::Keep) keep_count--;
            else if (decision == TinderDecision::Delete) delete_count--;
            else if (decision == TinderDecision::Skip) skip_count--;
            decision = TinderDecision::Pending;
        }
    }
    
    int execute_deletions() {
        int deleted = 0;
        for (size_t i = 0; i < files.size(); ++i) {
            if (decisions[i] == TinderDecision::Delete) {
                try {
                    std::filesystem::remove(files[i].full_path);
                    deleted++;
                } catch (...) {
                    // Ignore errors
                }
            }
        }
        return deleted;
    }
    
    std::string get_file_info() const {
        if (files.empty() || current_index >= static_cast<int>(files.size())) {
            return "No files";
        }
        
        const auto& file = files[current_index];
        std::string info;
        
        try {
            auto size = std::filesystem::file_size(file.full_path);
            std::string size_str;
            if (size < 1024) {
                size_str = std::to_string(size) + " B";
            } else if (size < 1024 * 1024) {
                size_str = std::to_string(size / 1024) + " KB";
            } else if (size < 1024 * 1024 * 1024) {
                size_str = std::to_string(size / (1024 * 1024)) + " MB";
            } else {
                size_str = std::to_string(size / (1024 * 1024 * 1024)) + " GB";
            }
            
            auto last_write = std::filesystem::last_write_time(file.full_path);
            
            info = "Size: " + size_str;
        } catch (...) {
            info = "Size: Unknown";
        }
        
        return info;
    }
    
    std::string get_file_preview() const {
        if (files.empty() || current_index >= static_cast<int>(files.size())) {
            return "";
        }
        
        const auto& file = files[current_index];
        std::string ext;
        size_t dot_pos = file.file_name.rfind('.');
        if (dot_pos != std::string::npos) {
            ext = file.file_name.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        }
        
        // Text file preview
        if (ext == "txt" || ext == "md" || ext == "log" || ext == "json" || ext == "xml" ||
            ext == "cpp" || ext == "h" || ext == "py" || ext == "js" || ext == "html" || ext == "css") {
            try {
                std::ifstream file_stream(file.full_path);
                if (file_stream) {
                    std::string content;
                    std::string line;
                    int line_count = 0;
                    while (std::getline(file_stream, line) && line_count < 10) {
                        content += line.substr(0, 60) + "\n";
                        line_count++;
                    }
                    return content;
                }
            } catch (...) {
                return "(Cannot preview file)";
            }
        }
        
        // Image files
        if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp") {
            return "[IMAGE FILE]\n\nPreview not available in TUI mode";
        }
        
        // Audio files
        if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "ogg") {
            return "[AUDIO FILE]\n\nPreview not available in TUI mode";
        }
        
        // Video files
        if (ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "mov") {
            return "[VIDEO FILE]\n\nPreview not available in TUI mode";
        }
        
        return "(No preview available)";
    }
};

} // namespace

Component TuiFileTinder::Create(
    const std::string& directory_path,
    DatabaseManager& db_manager,
    CloseCallback on_close) {
    
    auto state = std::make_shared<TinderState>(directory_path, db_manager, std::move(on_close));
    
    // Buttons
    auto keep_button = Button(" ← Keep (←) ", [state] { state->mark_keep(); });
    auto delete_button = Button(" Delete (→) → ", [state] { state->mark_delete(); });
    auto skip_button = Button(" Skip (↓) ", [state] { state->mark_skip(); });
    auto undo_button = Button(" Undo (U) ", [state] { state->undo_last(); });
    auto execute_button = Button(" Execute Deletions ", [state] { state->show_confirm_delete = true; });
    auto close_button = Button(" Close ", [state] { 
        if (state->on_close) state->on_close(); 
    });
    
    auto confirm_yes = Button(" Yes, Delete ", [state] {
        int deleted = state->execute_deletions();
        state->status_message = "Deleted " + std::to_string(deleted) + " files";
        state->show_confirm_delete = false;
        state->scan_files();
    });
    
    auto confirm_no = Button(" Cancel ", [state] {
        state->show_confirm_delete = false;
    });
    
    // Main renderer
    auto main_renderer = Renderer([state] {
        if (state->files.empty()) {
            return vbox({
                text("════════════════════════════════════════") | bold | center,
                text("           FILE TINDER                  ") | bold | center,
                text("════════════════════════════════════════") | bold | center,
                text(""),
                text("  No files found in directory") | center,
                text(""),
            });
        }
        
        const auto& current_file = state->files[state->current_index];
        std::string decision_str = "Pending";
        Color decision_color = Color::White;
        
        switch (state->decisions[state->current_index]) {
            case TinderDecision::Keep:
                decision_str = "KEEP";
                decision_color = Color::Green;
                break;
            case TinderDecision::Delete:
                decision_str = "DELETE";
                decision_color = Color::Red;
                break;
            case TinderDecision::Skip:
                decision_str = "SKIP";
                decision_color = Color::Yellow;
                break;
            default:
                break;
        }
        
        return vbox({
            text("════════════════════════════════════════") | bold | center,
            text("           FILE TINDER                  ") | bold | center,
            text("════════════════════════════════════════") | bold | center,
            text(""),
            hbox({
                text(" File ") | bold,
                text(std::to_string(state->current_index + 1) + " / " + std::to_string(state->files.size())),
                text(" | "),
                text("Keep: " + std::to_string(state->keep_count)) | color(Color::Green),
                text(" | "),
                text("Delete: " + std::to_string(state->delete_count)) | color(Color::Red),
                text(" | "),
                text("Skip: " + std::to_string(state->skip_count)) | color(Color::Yellow),
            }) | center,
            separator(),
            text(""),
            text(" " + current_file.file_name) | bold | size(WIDTH, EQUAL, 50) | center,
            text(" " + state->get_file_info()) | dim | center,
            text(" Decision: " + decision_str) | color(decision_color) | center,
            text(""),
            separator(),
            text(" Preview: ") | bold,
            text(state->get_file_preview()) | size(HEIGHT, EQUAL, 8) | frame,
            separator(),
            text(""),
            text(" ← Keep | ↓ Skip | → Delete | U Undo | ESC Close ") | dim | center,
        });
    });
    
    // Confirm dialog
    auto confirm_dialog = Container::Vertical({
        Renderer([state] {
            return vbox({
                text(" Confirm Deletion ") | bold | center,
                text(""),
                text(" Are you sure you want to delete " + std::to_string(state->delete_count) + " files?") | center,
                text(" This action cannot be undone! ") | color(Color::Red) | center,
                text(""),
            });
        }),
        Container::Horizontal({
            confirm_yes,
            confirm_no,
        }) | center,
    }) | border;
    
    // Main container with buttons
    auto main_container = Container::Vertical({
        main_renderer,
        Container::Horizontal({
            keep_button,
            skip_button,
            delete_button,
        }) | center,
        Container::Horizontal({
            undo_button,
            execute_button,
            close_button,
        }) | center,
    });
    
    // Handle keyboard events
    auto with_events = CatchEvent(main_container, [state](Event event) {
        if (event == Event::ArrowLeft) {
            state->mark_keep();
            return true;
        }
        if (event == Event::ArrowRight) {
            state->mark_delete();
            return true;
        }
        if (event == Event::ArrowDown) {
            state->mark_skip();
            return true;
        }
        if (event == Event::ArrowUp) {
            state->move_prev();
            return true;
        }
        if (event == Event::Character('u') || event == Event::Character('U')) {
            state->undo_last();
            return true;
        }
        if (event == Event::Escape) {
            if (state->show_confirm_delete) {
                state->show_confirm_delete = false;
            } else if (state->on_close) {
                state->on_close();
            }
            return true;
        }
        return false;
    });
    
    return Modal(with_events, confirm_dialog, &state->show_confirm_delete) | border;
}
