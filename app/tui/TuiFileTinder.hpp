#ifndef TUI_FILE_TINDER_HPP
#define TUI_FILE_TINDER_HPP

#include "DatabaseManager.hpp"
#include "Types.hpp"

#include <ftxui/component/component.hpp>
#include <functional>
#include <vector>
#include <string>

/**
 * TUI component for File Tinder (swipe-style file cleanup)
 */
class TuiFileTinder {
public:
    using CloseCallback = std::function<void()>;
    
    static ftxui::Component Create(
        const std::string& directory_path,
        DatabaseManager& db_manager,
        CloseCallback on_close);
};

#endif // TUI_FILE_TINDER_HPP
