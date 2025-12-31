# Phase 2: Dynamic Category Wizard Implementation

## Overview

Implements dynamic category creation wizard with confidence-based triggering, allowing users to create new categories on-the-fly during categorization when AI is uncertain or no good match exists.

## Core Features

### 1. Confidence-Based Triggering

**Trigger Conditions:**
- AI confidence score < threshold (default: 0.7, configurable)
- AI returns "UNCERTAIN" response
- AI picks generic/fallback category ("Uncategorized", "Miscellaneous", "Other", "Unknown")
- Whitelist enforcement finds no valid path

**Settings:**
```cpp
// New settings
bool enable_category_wizard = false;  // Default: OFF (opt-in feature)
double wizard_confidence_threshold = 0.7;  // Trigger below this score
```

### 2. CategorySuggestionWizard Dialog

**UI Components:**
- File preview area (thumbnail for images, text snippet for text files)
- AI suggested parent category (if available)
- Radio button group:
  - Use existing parent category (place file at parent level)
  - Create new subcategory under parent (input field)
  - Skip this file
- Checkbox: "Apply to similar files in this batch"
- Buttons: "Create & Continue", "Cancel"

**File Preview:**
- Images: Thumbnail (max 200x200, maintain aspect ratio)
- Text files: First 500 chars with syntax highlighting
- Other files: Icon + filename + size + modified date

### 3. Implementation Files

#### New Files:
1. **app/include/CategorySuggestionWizard.hpp** (~100 lines)
   - Dialog class definition
   - Result enum (CreateNew, UseParent, Skip)
   - Preview widget

2. **app/lib/CategorySuggestionWizard.cpp** (~250 lines)
   - Dialog implementation
   - File preview logic
   - Input validation

#### Modified Files:
1. **app/include/Settings.hpp** (+15 lines)
   - `get_enable_category_wizard()`, `set_enable_category_wizard()`
   - `get_wizard_confidence_threshold()`, `set_wizard_confidence_threshold()`

2. **app/lib/Settings.cpp** (+30 lines)
   - Implementation of wizard settings

3. **app/include/CategorizationService.hpp** (+20 lines)
   - `show_category_wizard()` method
   - `add_path_to_whitelist()` method
   - `should_trigger_wizard()` helper

4. **app/lib/CategorizationService.cpp** (+150 lines)
   - Wizard integration in `categorize_single_entry()`
   - Dynamic whitelist update logic
   - Confidence score parsing and evaluation
   - Batch learning ("apply to similar" functionality)

5. **app/include/WhitelistStore.hpp** (+10 lines)
   - `add_path()` method
   - `has_path()` method

6. **app/lib/WhitelistStore.cpp** (+50 lines)
   - Dynamic path addition to tree structure
   - Save after modification

7. **app/include/MainApp.hpp** (+5 lines)
   - Settings dialog integration

8. **app/lib/MainApp.cpp** (+20 lines)
   - Update Settings UI with wizard options

9. **app/include/CategorizationProgressDialog.hpp** (+5 lines)
   - Wizard interaction state display

10. **app/lib/CategorizationProgressDialog.cpp** (+30 lines)
    - Show "Waiting for user input..." when wizard appears
    - Display newly created categories

11. **app/include/UiTranslator.hpp** (+10 lines)
    - New translation keys for wizard

12. **app/lib/UiTranslator.cpp** (+50 lines)
    - Translation strings for all wizard UI elements

### 4. Integration Flow

```cpp
// In CategorizationService::categorize_single_entry()

DatabaseManager::ResolvedCategory result = /* AI categorization */;

// Check if wizard should be triggered
if (should_trigger_wizard(result, confidence_score)) {
    if (settings_.get_enable_category_wizard()) {
        // Show wizard
        CategorySuggestionWizard wizard(
            entry,
            result.category,  // Suggested parent
            whitelist_store_.get_all_paths(),
            parent_widget_
        );
        
        if (wizard.exec() == QDialog::Accepted) {
            auto wizard_result = wizard.get_result();
            
            switch (wizard_result) {
                case CategorySuggestionWizard::CreateNew: {
                    std::string new_path = wizard.get_path();
                    
                    // Add to whitelist
                    add_path_to_whitelist(new_path);
                    
                    // Resolve and return
                    result = resolve_path(new_path);
                    
                    // Batch learning
                    if (wizard.apply_to_similar()) {
                        batch_wizard_rules_[entry.extension] = new_path;
                    }
                    break;
                }
                case CategorySuggestionWizard::UseParent: {
                    result = resolve_path(wizard.get_parent_path());
                    break;
                }
                case CategorySuggestionWizard::Skip: {
                    return DatabaseManager::ResolvedCategory{-1, "", ""};
                }
            }
        } else {
            // User cancelled wizard
            return DatabaseManager::ResolvedCategory{-1, "", ""};
        }
    } else {
        // Wizard disabled, fall back to uncategorized
        return DatabaseManager::ResolvedCategory{-1, "", ""};
    }
}

return result;
```

### 5. should_trigger_wizard() Logic

```cpp
bool CategorizationService::should_trigger_wizard(
    const DatabaseManager::ResolvedCategory& result,
    double confidence_score
) {
    // Condition 1: Low confidence
    if (confidence_score >= 0 && 
        confidence_score < settings_.get_wizard_confidence_threshold()) {
        return true;
    }
    
    // Condition 2: Uncategorized/Generic
    if (result.category.empty() || 
        is_generic_category(result.category)) {
        return true;
    }
    
    // Condition 3: UNCERTAIN flag
    if (result.id == -1 && result.category.empty()) {
        return true;
    }
    
    return false;
}
```

### 6. add_path_to_whitelist() Implementation

```cpp
void CategorizationService::add_path_to_whitelist(const std::string& path) {
    auto entry = whitelist_store_.get(whitelist_store_.default_name());
    
    // Split path into segments
    std::vector<std::string> segments;
    std::istringstream iss(path);
    std::string segment;
    while (std::getline(iss, segment, '/')) {
        if (!segment.empty()) {
            segments.push_back(segment);
        }
    }
    
    if (segments.empty()) return;
    
    // Navigate/create tree structure
    std::vector<CategoryNode>* current_children = &entry.category_nodes;
    
    for (const auto& seg : segments) {
        auto it = std::find_if(
            current_children->begin(),
            current_children->end(),
            [&](const CategoryNode& n) { return n.name == seg; }
        );
        
        if (it == current_children->end()) {
            // Create new node
            CategoryNode new_node;
            new_node.name = seg;
            current_children->push_back(new_node);
            it = current_children->end() - 1;
        }
        
        current_children = &it->children;
    }
    
    // Save immediately
    whitelist_store_.set(whitelist_store_.default_name(), entry);
    whitelist_store_.save();
    
    // Refresh cached paths
    refresh_allowed_paths();
}
```

### 7. UI Text Updates

**Settings Dialog:**
```
Category Wizard
  ☐ Enable category suggestion wizard
      Prompt to create new categories when AI is uncertain

  Confidence Threshold: [====|----] 0.70
      Trigger wizard when AI confidence is below this value
```

**Wizard Dialog:**
```
Title: "Create New Category?"

File: vacation_photo.jpg
[Preview Image]

AI Suggestion: Photos (confidence: 0.45)

○ Use parent category: Photos
● Create new subcategory:
  Photos / [Vacation     ]

○ Skip this file

☑ Apply to similar files (.jpg) in this batch

[Create & Continue]  [Cancel]
```

**Progress Dialog Updates:**
```
Categorizing: 45 / 120 files
Current: vacation_photo.jpg

Status: Waiting for user input...

Recently created:
  • Photos/Vacation (3 files)
  • Documents/Receipts (1 file)
```

### 8. Batch Learning

**Similar Files Detection:**
- Same file extension
- Similar file size (±20%)
- Same parent folder
- Similar filename pattern (regex-based)

**Auto-Apply Rules:**
```cpp
// Store wizard decisions per file type
std::map<std::string, std::string> batch_wizard_rules_;

// Check before showing wizard
if (batch_wizard_rules_.count(entry.extension)) {
    auto path = batch_wizard_rules_[entry.extension];
    return resolve_path(path);
}
```

### 9. Error Handling

**Invalid Input:**
- Empty category name
- Invalid characters (filesystem-unsafe)
- Duplicate paths
- Path length > 255 chars
- Too many levels (> 10)

**User Feedback:**
```cpp
// Validation in wizard
QString CategorySuggestionWizard::validate_input() {
    QString input = subcategory_input_->text().trimmed();
    
    if (input.isEmpty()) {
        return "Category name cannot be empty";
    }
    
    if (!is_valid_path_segment(input.toStdString())) {
        return "Category name contains invalid characters";
    }
    
    std::string full_path = parent_ + "/" + input.toStdString();
    if (full_path.length() > 255) {
        return "Path is too long";
    }
    
    if (count_path_depth(full_path) > 10) {
        return "Maximum nesting depth (10) exceeded";
    }
    
    return "";  // Valid
}
```

### 10. Testing

**Unit Tests:**
```cpp
TEST(CategoryWizardTest, TriggerOnLowConfidence) {
    service.set_wizard_enabled(true);
    service.set_confidence_threshold(0.7);
    
    auto result = ResolvedCategory{1, "Photos", ""};
    EXPECT_TRUE(service.should_trigger_wizard(result, 0.5));
    EXPECT_FALSE(service.should_trigger_wizard(result, 0.8));
}

TEST(CategoryWizardTest, DynamicPathAddition) {
    service.add_path_to_whitelist("Audio/Samples/Drums");
    
    auto paths = whitelist_store.get_all_paths();
    EXPECT_THAT(paths, Contains("Audio"));
    EXPECT_THAT(paths, Contains("Audio/Samples"));
    EXPECT_THAT(paths, Contains("Audio/Samples/Drums"));
}
```

**Integration Tests:**
```cpp
TEST(CategoryWizardIntegrationTest, EndToEndFlow) {
    enable_wizard();
    mock_wizard_input("Photos/Vacation");
    
    auto files = {FileEntry{"test.jpg"}};
    auto results = service.categorize(files);
    
    EXPECT_EQ(results[0].category, "Photos");
    EXPECT_EQ(results[0].subcategory, "Vacation");
    
    // Verify persisted
    auto paths = get_whitelist_paths();
    EXPECT_THAT(paths, Contains("Photos/Vacation"));
}
```

## Timeline

**Day 1-2:** Core wizard dialog + file preview
**Day 3:** Integration with CategorizationService
**Day 4:** Settings UI + confidence integration
**Day 5:** Batch learning + UI text updates
**Day 6:** Testing + bug fixes
**Day 7:** Documentation + polish

## Success Criteria

- ✅ Wizard appears on low confidence / uncertain / generic categories
- ✅ Users can create new categories dynamically
- ✅ New categories instantly available for remaining files
- ✅ Batch learning reduces wizard appearances
- ✅ Settings properly control wizard behavior
- ✅ All UI text clear and consistent
- ✅ No breaking changes to existing functionality
- ✅ Performance impact < 5%
