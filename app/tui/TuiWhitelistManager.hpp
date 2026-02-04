#ifndef TUI_WHITELIST_MANAGER_HPP
#define TUI_WHITELIST_MANAGER_HPP

#include "TuiSettings.hpp"

#include <ftxui/component/component.hpp>
#include <functional>
#include <vector>
#include <string>

/**
 * TUI component for managing category whitelists
 */
class TuiWhitelistManager {
public:
    using CloseCallback = std::function<void()>;
    
    static ftxui::Component Create(TuiSettings& settings, CloseCallback on_close);
};

#endif // TUI_WHITELIST_MANAGER_HPP
