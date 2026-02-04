#ifndef TUI_CATEGORIZATION_RESULTS_HPP
#define TUI_CATEGORIZATION_RESULTS_HPP

#include "DatabaseManager.hpp"
#include "Types.hpp"

#include <ftxui/component/component.hpp>
#include <functional>
#include <vector>
#include <string>

/**
 * TUI component for displaying categorization results
 */
class TuiCategorizationResults {
public:
    using CloseCallback = std::function<void()>;
    using SortCallback = std::function<void(const std::vector<CategorizedFile>&)>;
    
    static ftxui::Component Create(
        std::vector<CategorizedFile>& files,
        DatabaseManager& db_manager,
        CloseCallback on_close,
        SortCallback on_sort);
};

#endif // TUI_CATEGORIZATION_RESULTS_HPP
