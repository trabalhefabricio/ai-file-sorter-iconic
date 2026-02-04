#ifndef TUI_LLM_SELECTION_HPP
#define TUI_LLM_SELECTION_HPP

#include "TuiSettings.hpp"
#include "Types.hpp"

#include <ftxui/component/component.hpp>
#include <functional>
#include <string>

/**
 * TUI component for LLM selection dialog
 */
class TuiLLMSelection {
public:
    using Callback = std::function<void(bool accepted)>;
    
    static ftxui::Component Create(TuiSettings& settings, Callback on_close);
    
private:
    TuiSettings& settings_;
    Callback on_close_;
    
    int selected_llm_{0};
    std::string openai_key_;
    std::string openai_model_;
    std::string gemini_key_;
    std::string gemini_model_;
    std::string custom_path_;
    std::string custom_name_;
    
    TuiLLMSelection(TuiSettings& settings, Callback on_close);
    ftxui::Component build();
    void apply_selection();
    void load_current_settings();
};

#endif // TUI_LLM_SELECTION_HPP
