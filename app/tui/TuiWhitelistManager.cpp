#include "TuiWhitelistManager.hpp"
#include "WhitelistStore.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>

using namespace ftxui;

namespace {

struct WhitelistState {
    TuiSettings& settings;
    TuiWhitelistManager::CloseCallback on_close;
    
    std::vector<std::string> whitelist_names;
    int selected_whitelist{0};
    std::vector<std::string> categories;
    std::vector<std::string> subcategories;
    int selected_category{0};
    int selected_subcategory{0};
    
    std::string new_whitelist_name;
    std::string new_category;
    std::string new_subcategory;
    
    bool show_add_whitelist{false};
    bool show_add_category{false};
    bool show_add_subcategory{false};
    std::string status_message;
    
    WhitelistStore store;
    
    WhitelistState(TuiSettings& s, TuiWhitelistManager::CloseCallback close_cb)
        : settings(s), on_close(std::move(close_cb)), store(s.get_config_dir()) {
        store.load();
        refresh_whitelists();
    }
    
    void refresh_whitelists() {
        whitelist_names = store.list_names();
        if (whitelist_names.empty()) {
            // Create default whitelist
            WhitelistEntry entry;
            entry.categories = {"Documents", "Images", "Audio", "Video", "Archives"};
            store.set("Default", entry);
            store.save();
            whitelist_names = store.list_names();
        }
        
        if (selected_whitelist >= static_cast<int>(whitelist_names.size())) {
            selected_whitelist = 0;
        }
        
        load_current_whitelist();
    }
    
    void load_current_whitelist() {
        if (whitelist_names.empty()) {
            categories.clear();
            subcategories.clear();
            return;
        }
        
        const auto& name = whitelist_names[selected_whitelist];
        auto entry = store.get(name);
        if (entry) {
            categories = entry->categories;
            subcategories = entry->subcategories;
        } else {
            categories.clear();
            subcategories.clear();
        }
        selected_category = 0;
        selected_subcategory = 0;
    }
    
    void save_current_whitelist() {
        if (whitelist_names.empty()) return;
        
        const auto& name = whitelist_names[selected_whitelist];
        WhitelistEntry entry;
        entry.categories = categories;
        entry.subcategories = subcategories;
        store.set(name, entry);
        store.save();
    }
    
    void add_whitelist() {
        if (new_whitelist_name.empty()) {
            status_message = "Whitelist name cannot be empty";
            return;
        }
        
        WhitelistEntry entry;
        store.set(new_whitelist_name, entry);
        store.save();
        new_whitelist_name.clear();
        show_add_whitelist = false;
        refresh_whitelists();
        status_message = "Whitelist created";
    }
    
    void delete_whitelist() {
        if (whitelist_names.empty()) return;
        
        const auto& name = whitelist_names[selected_whitelist];
        store.remove(name);
        store.save();
        refresh_whitelists();
        status_message = "Whitelist deleted";
    }
    
    void add_category() {
        if (new_category.empty() || whitelist_names.empty()) {
            status_message = "Category name cannot be empty";
            return;
        }
        
        categories.push_back(new_category);
        save_current_whitelist();
        new_category.clear();
        show_add_category = false;
        status_message = "Category added";
    }
    
    void delete_category() {
        if (categories.empty() || selected_category >= static_cast<int>(categories.size())) return;
        
        categories.erase(categories.begin() + selected_category);
        save_current_whitelist();
        if (selected_category >= static_cast<int>(categories.size()) && !categories.empty()) {
            selected_category = static_cast<int>(categories.size()) - 1;
        }
        status_message = "Category deleted";
    }
    
    void add_subcategory() {
        if (new_subcategory.empty() || whitelist_names.empty()) {
            status_message = "Subcategory name cannot be empty";
            return;
        }
        
        subcategories.push_back(new_subcategory);
        save_current_whitelist();
        new_subcategory.clear();
        show_add_subcategory = false;
        status_message = "Subcategory added";
    }
    
    void delete_subcategory() {
        if (subcategories.empty() || selected_subcategory >= static_cast<int>(subcategories.size())) return;
        
        subcategories.erase(subcategories.begin() + selected_subcategory);
        save_current_whitelist();
        if (selected_subcategory >= static_cast<int>(subcategories.size()) && !subcategories.empty()) {
            selected_subcategory = static_cast<int>(subcategories.size()) - 1;
        }
        status_message = "Subcategory deleted";
    }
    
    void activate_current() {
        if (whitelist_names.empty()) return;
        settings.set_active_whitelist(whitelist_names[selected_whitelist]);
        settings.set_use_whitelist(true);
        settings.save();
        status_message = "Whitelist '" + whitelist_names[selected_whitelist] + "' activated";
    }
};

} // namespace

Component TuiWhitelistManager::Create(TuiSettings& settings, CloseCallback on_close) {
    auto state = std::make_shared<WhitelistState>(settings, std::move(on_close));
    
    // Inputs
    auto whitelist_name_input = Input(&state->new_whitelist_name, "Whitelist name...");
    auto category_input = Input(&state->new_category, "Category name...");
    auto subcategory_input = Input(&state->new_subcategory, "Subcategory name...");
    
    // Whitelist list
    auto whitelist_menu = Menu(&state->whitelist_names, &state->selected_whitelist);
    
    // Category list
    auto category_menu = Menu(&state->categories, &state->selected_category);
    
    // Subcategory list
    auto subcategory_menu = Menu(&state->subcategories, &state->selected_subcategory);
    
    // Buttons
    auto add_wl_button = Button(" + Whitelist ", [state] { state->show_add_whitelist = true; });
    auto del_wl_button = Button(" - Whitelist ", [state] { state->delete_whitelist(); });
    auto activate_button = Button(" Activate ", [state] { state->activate_current(); });
    
    auto add_cat_button = Button(" + Category ", [state] { state->show_add_category = true; });
    auto del_cat_button = Button(" - Category ", [state] { state->delete_category(); });
    
    auto add_sub_button = Button(" + Subcategory ", [state] { state->show_add_subcategory = true; });
    auto del_sub_button = Button(" - Subcategory ", [state] { state->delete_subcategory(); });
    
    auto close_button = Button(" Close ", [state] {
        if (state->on_close) state->on_close();
    });
    
    // Add whitelist dialog
    auto add_wl_ok = Button(" OK ", [state] { state->add_whitelist(); });
    auto add_wl_cancel = Button(" Cancel ", [state] { state->show_add_whitelist = false; });
    
    auto add_wl_dialog = Container::Vertical({
        Renderer([] { return text(" New Whitelist ") | bold | center; }),
        whitelist_name_input,
        Container::Horizontal({ add_wl_ok, add_wl_cancel }) | center,
    }) | border;
    
    // Add category dialog
    auto add_cat_ok = Button(" OK ", [state] { state->add_category(); });
    auto add_cat_cancel = Button(" Cancel ", [state] { state->show_add_category = false; });
    
    auto add_cat_dialog = Container::Vertical({
        Renderer([] { return text(" New Category ") | bold | center; }),
        category_input,
        Container::Horizontal({ add_cat_ok, add_cat_cancel }) | center,
    }) | border;
    
    // Add subcategory dialog
    auto add_sub_ok = Button(" OK ", [state] { state->add_subcategory(); });
    auto add_sub_cancel = Button(" Cancel ", [state] { state->show_add_subcategory = false; });
    
    auto add_sub_dialog = Container::Vertical({
        Renderer([] { return text(" New Subcategory ") | bold | center; }),
        subcategory_input,
        Container::Horizontal({ add_sub_ok, add_sub_cancel }) | center,
    }) | border;
    
    // Update whitelist selection handler
    whitelist_menu = whitelist_menu | CatchEvent([state](Event event) {
        if (event == Event::Return) {
            state->load_current_whitelist();
            return true;
        }
        return false;
    });
    
    // Main layout
    auto main_container = Container::Vertical({
        Renderer([] {
            return vbox({
                text("════════════════════════════════════════════════") | bold | center,
                text("           WHITELIST MANAGER                    ") | bold | center,
                text("════════════════════════════════════════════════") | bold | center,
            });
        }),
        Container::Horizontal({
            // Whitelists panel
            Container::Vertical({
                Renderer([] { return text(" Whitelists ") | bold; }),
                whitelist_menu | size(HEIGHT, EQUAL, 8) | frame | border,
                Container::Horizontal({ add_wl_button, del_wl_button }),
                activate_button | center,
            }) | size(WIDTH, EQUAL, 20),
            
            // Categories panel
            Container::Vertical({
                Renderer([] { return text(" Categories ") | bold; }),
                category_menu | size(HEIGHT, EQUAL, 8) | frame | border,
                Container::Horizontal({ add_cat_button, del_cat_button }),
            }) | size(WIDTH, EQUAL, 20),
            
            // Subcategories panel
            Container::Vertical({
                Renderer([] { return text(" Subcategories ") | bold; }),
                subcategory_menu | size(HEIGHT, EQUAL, 8) | frame | border,
                Container::Horizontal({ add_sub_button, del_sub_button }),
            }) | size(WIDTH, EQUAL, 20),
        }),
        Renderer([state] {
            std::string active = state->settings.get_active_whitelist();
            return vbox({
                separator(),
                text(" Active: " + (active.empty() ? "(none)" : active)) | color(Color::Green),
                text(" " + state->status_message) | dim,
            });
        }),
        close_button | center,
    });
    
    // Handle keyboard
    auto with_events = CatchEvent(main_container, [state](Event event) {
        if (event == Event::Escape) {
            if (state->show_add_whitelist) {
                state->show_add_whitelist = false;
            } else if (state->show_add_category) {
                state->show_add_category = false;
            } else if (state->show_add_subcategory) {
                state->show_add_subcategory = false;
            } else if (state->on_close) {
                state->on_close();
            }
            return true;
        }
        return false;
    });
    
    // Add modals
    auto with_modals = Modal(with_events, add_wl_dialog, &state->show_add_whitelist);
    with_modals = Modal(with_modals, add_cat_dialog, &state->show_add_category);
    with_modals = Modal(with_modals, add_sub_dialog, &state->show_add_subcategory);
    
    return with_modals | border;
}
