#include "TuiLLMSelection.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

TuiLLMSelection::TuiLLMSelection(TuiSettings& settings, Callback on_close)
    : settings_(settings), on_close_(std::move(on_close))
{
    load_current_settings();
}

void TuiLLMSelection::load_current_settings() {
    // Map current LLM choice to selection index
    switch (settings_.get_llm_choice()) {
        case LLMChoice::Local_3b: selected_llm_ = 0; break;
        case LLMChoice::Local_7b: selected_llm_ = 1; break;
        case LLMChoice::Remote_OpenAI: selected_llm_ = 2; break;
        case LLMChoice::Remote_Gemini: selected_llm_ = 3; break;
        case LLMChoice::Custom: selected_llm_ = 4; break;
        default: selected_llm_ = 1; break; // Default to Local 7B
    }
    
    openai_key_ = settings_.get_openai_api_key();
    openai_model_ = settings_.get_openai_model();
    gemini_key_ = settings_.get_gemini_api_key();
    gemini_model_ = settings_.get_gemini_model();
    
    auto custom_llms = settings_.get_custom_llms();
    if (!custom_llms.empty()) {
        custom_path_ = custom_llms[0].path;
        custom_name_ = custom_llms[0].name;
    }
}

void TuiLLMSelection::apply_selection() {
    LLMChoice choice;
    switch (selected_llm_) {
        case 0: choice = LLMChoice::Local_3b; break;
        case 1: choice = LLMChoice::Local_7b; break;
        case 2: choice = LLMChoice::Remote_OpenAI; break;
        case 3: choice = LLMChoice::Remote_Gemini; break;
        case 4: choice = LLMChoice::Custom; break;
        default: choice = LLMChoice::Local_7b; break;
    }
    
    settings_.set_llm_choice(choice);
    settings_.set_openai_api_key(openai_key_);
    settings_.set_openai_model(openai_model_);
    settings_.set_gemini_api_key(gemini_key_);
    settings_.set_gemini_model(gemini_model_);
    
    if (!custom_path_.empty() && !custom_name_.empty()) {
        CustomLLM custom;
        custom.name = custom_name_;
        custom.path = custom_path_;
        custom.description = "Custom LLM";
        settings_.upsert_custom_llm(custom);
    }
    
    settings_.save();
}

Component TuiLLMSelection::build() {
    std::vector<std::string> llm_options = {
        "Local LLM (3B) - Lightweight, faster",
        "Local LLM (7B) - More accurate",
        "ChatGPT (OpenAI API)",
        "Gemini (Google AI)",
        "Custom LLM (GGUF file)",
    };
    
    auto radiobox = Radiobox(&llm_options, &selected_llm_);
    
    // API key inputs
    auto openai_key_input = Input(&openai_key_, "Enter OpenAI API key...");
    auto openai_model_input = Input(&openai_model_, "gpt-4o-mini");
    auto gemini_key_input = Input(&gemini_key_, "Enter Gemini API key...");
    auto gemini_model_input = Input(&gemini_model_, "gemini-2.5-flash-lite");
    auto custom_path_input = Input(&custom_path_, "Path to GGUF file...");
    auto custom_name_input = Input(&custom_name_, "Custom LLM name...");
    
    // Buttons
    auto ok_button = Button(" OK ", [this] {
        apply_selection();
        on_close_(true);
    });
    
    auto cancel_button = Button(" Cancel ", [this] {
        on_close_(false);
    });
    
    // Build the complete dialog
    auto container = Container::Vertical({
        Renderer([] {
            return vbox({
                text("═══════════════════════════════════════") | bold | center,
                text("         SELECT LLM MODEL              ") | bold | center,
                text("═══════════════════════════════════════") | bold | center,
                text(""),
            });
        }),
        radiobox,
        Renderer([] { return separator(); }),
        
        // OpenAI section (shown when OpenAI selected)
        Container::Vertical({
            Renderer([] { return text(" OpenAI Settings: ") | bold; }),
            Container::Horizontal({
                Renderer([] { return text(" API Key: "); }),
                openai_key_input | flex,
            }),
            Container::Horizontal({
                Renderer([] { return text(" Model:   "); }),
                openai_model_input | flex,
            }),
        }) | Maybe([this] { return selected_llm_ == 2; }),
        
        // Gemini section (shown when Gemini selected)
        Container::Vertical({
            Renderer([] { return text(" Gemini Settings: ") | bold; }),
            Container::Horizontal({
                Renderer([] { return text(" API Key: "); }),
                gemini_key_input | flex,
            }),
            Container::Horizontal({
                Renderer([] { return text(" Model:   "); }),
                gemini_model_input | flex,
            }),
        }) | Maybe([this] { return selected_llm_ == 3; }),
        
        // Custom LLM section (shown when Custom selected)
        Container::Vertical({
            Renderer([] { return text(" Custom LLM Settings: ") | bold; }),
            Container::Horizontal({
                Renderer([] { return text(" Name: "); }),
                custom_name_input | flex,
            }),
            Container::Horizontal({
                Renderer([] { return text(" Path: "); }),
                custom_path_input | flex,
            }),
        }) | Maybe([this] { return selected_llm_ == 4; }),
        
        Renderer([] { return separator(); }),
        Container::Horizontal({
            ok_button,
            cancel_button,
        }) | center,
    });
    
    return container | border | size(WIDTH, EQUAL, 50) | size(HEIGHT, LESS_THAN, 25);
}

Component TuiLLMSelection::Create(TuiSettings& settings, Callback on_close) {
    auto instance = std::make_shared<TuiLLMSelection>(settings, std::move(on_close));
    return instance->build();
}
