#include "TuiCategorizationResults.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <algorithm>
#include <map>

using namespace ftxui;

namespace {

struct ResultsState {
    std::vector<CategorizedFile>& files;
    DatabaseManager& db_manager;
    TuiCategorizationResults::CloseCallback on_close;
    TuiCategorizationResults::SortCallback on_sort;
    
    int selected_index{0};
    int scroll_offset{0};
    bool editing_category{false};
    std::string edit_category;
    std::string edit_subcategory;
    std::string status_message;
    
    ResultsState(std::vector<CategorizedFile>& f, DatabaseManager& db,
                 TuiCategorizationResults::CloseCallback close_cb,
                 TuiCategorizationResults::SortCallback sort_cb)
        : files(f), db_manager(db), on_close(std::move(close_cb)), on_sort(std::move(sort_cb)) {}
};

} // namespace

Component TuiCategorizationResults::Create(
    std::vector<CategorizedFile>& files,
    DatabaseManager& db_manager,
    CloseCallback on_close,
    SortCallback on_sort) {
    
    auto state = std::make_shared<ResultsState>(files, db_manager, std::move(on_close), std::move(on_sort));
    
    // Category edit inputs
    auto category_input = Input(&state->edit_category, "Category...");
    auto subcategory_input = Input(&state->edit_subcategory, "Subcategory...");
    
    // Buttons
    auto sort_button = Button(" Sort Files ", [state] {
        if (state->on_sort) {
            state->on_sort(state->files);
        }
        state->status_message = "Files sorted!";
    });
    
    auto close_button = Button(" Close ", [state] {
        if (state->on_close) {
            state->on_close();
        }
    });
    
    auto edit_button = Button(" Edit ", [state] {
        if (state->selected_index >= 0 && state->selected_index < static_cast<int>(state->files.size())) {
            state->editing_category = true;
            state->edit_category = state->files[state->selected_index].category;
            state->edit_subcategory = state->files[state->selected_index].subcategory;
        }
    });
    
    auto save_edit_button = Button(" Save ", [state] {
        if (state->selected_index >= 0 && state->selected_index < static_cast<int>(state->files.size())) {
            state->files[state->selected_index].category = state->edit_category;
            state->files[state->selected_index].subcategory = state->edit_subcategory;
            state->editing_category = false;
            state->status_message = "Category updated";
        }
    });
    
    auto cancel_edit_button = Button(" Cancel ", [state] {
        state->editing_category = false;
    });
    
    // Main results renderer
    auto results_renderer = Renderer([state] {
        std::vector<Element> rows;
        
        // Header
        rows.push_back(hbox({
            text(" # ") | size(WIDTH, EQUAL, 4),
            separator(),
            text(" File Name ") | size(WIDTH, EQUAL, 30),
            separator(),
            text(" Category ") | size(WIDTH, EQUAL, 15),
            separator(),
            text(" Subcategory ") | size(WIDTH, EQUAL, 15),
            separator(),
            text(" Type "),
        }) | bold | bgcolor(Color::Blue));
        
        rows.push_back(separator());
        
        // File entries
        const int max_visible = 15;
        int start_idx = state->scroll_offset;
        int end_idx = std::min(start_idx + max_visible, static_cast<int>(state->files.size()));
        
        for (int i = start_idx; i < end_idx; ++i) {
            const auto& file = state->files[i];
            bool selected = (i == state->selected_index);
            
            auto row = hbox({
                text(std::to_string(i + 1)) | size(WIDTH, EQUAL, 4),
                separator(),
                text(file.file_name.substr(0, 28)) | size(WIDTH, EQUAL, 30),
                separator(),
                text(file.category) | size(WIDTH, EQUAL, 15) | color(Color::Green),
                separator(),
                text(file.subcategory) | size(WIDTH, EQUAL, 15) | color(Color::Yellow),
                separator(),
                text(file.type == FileType::Directory ? "DIR" : "FILE"),
            });
            
            if (selected) {
                row = row | bgcolor(Color::GrayDark) | bold;
            }
            if (file.from_cache) {
                row = row | dim;
            }
            
            rows.push_back(row);
        }
        
        return vbox(rows);
    });
    
    // Summary renderer
    auto summary_renderer = Renderer([state] {
        std::map<std::string, int> category_counts;
        for (const auto& file : state->files) {
            category_counts[file.category]++;
        }
        
        std::vector<Element> summary;
        summary.push_back(text(" Category Summary: ") | bold);
        for (const auto& [cat, count] : category_counts) {
            summary.push_back(text("  " + cat + ": " + std::to_string(count)));
        }
        
        return vbox(summary);
    });
    
    // Edit dialog
    auto edit_dialog = Container::Vertical({
        Renderer([] { return text(" Edit Category ") | bold | center; }),
        Container::Horizontal({
            Renderer([] { return text(" Category:    "); }),
            category_input | flex,
        }),
        Container::Horizontal({
            Renderer([] { return text(" Subcategory: "); }),
            subcategory_input | flex,
        }),
        Container::Horizontal({
            save_edit_button,
            cancel_edit_button,
        }) | center,
    }) | border | size(WIDTH, EQUAL, 40);
    
    // Main container
    auto main_container = Container::Vertical({
        Renderer([] {
            return vbox({
                text("════════════════════════════════════════════════════════════") | bold | center,
                text("              CATEGORIZATION RESULTS                        ") | bold | center,
                text("════════════════════════════════════════════════════════════") | bold | center,
            });
        }),
        results_renderer | flex,
        Renderer([] { return separator(); }),
        Container::Horizontal({
            summary_renderer | size(WIDTH, EQUAL, 30),
            Renderer([state] {
                return vbox({
                    text(" Selected: " + (state->selected_index < static_cast<int>(state->files.size()) ? 
                          state->files[state->selected_index].file_name : "None")),
                    text(" " + state->status_message) | color(Color::Green),
                });
            }) | flex,
        }),
        Renderer([] { return separator(); }),
        Renderer([] {
            return text(" Use ↑↓ to navigate, E to edit, Enter to sort, ESC to close ") | dim | center;
        }),
        Container::Horizontal({
            edit_button,
            sort_button,
            close_button,
        }) | center,
    });
    
    // Handle keyboard events
    auto with_events = CatchEvent(main_container, [state](Event event) {
        if (event == Event::ArrowUp && state->selected_index > 0) {
            state->selected_index--;
            if (state->selected_index < state->scroll_offset) {
                state->scroll_offset = state->selected_index;
            }
            return true;
        }
        if (event == Event::ArrowDown && state->selected_index < static_cast<int>(state->files.size()) - 1) {
            state->selected_index++;
            if (state->selected_index >= state->scroll_offset + 15) {
                state->scroll_offset = state->selected_index - 14;
            }
            return true;
        }
        if (event == Event::Character('e') || event == Event::Character('E')) {
            if (state->selected_index >= 0 && state->selected_index < static_cast<int>(state->files.size())) {
                state->editing_category = true;
                state->edit_category = state->files[state->selected_index].category;
                state->edit_subcategory = state->files[state->selected_index].subcategory;
            }
            return true;
        }
        if (event == Event::Escape) {
            if (state->editing_category) {
                state->editing_category = false;
            } else if (state->on_close) {
                state->on_close();
            }
            return true;
        }
        return false;
    });
    
    // Add edit dialog as modal
    return Modal(with_events, edit_dialog, &state->editing_category) | border;
}
