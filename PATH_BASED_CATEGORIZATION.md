# Path-Based Categorization with Dynamic Wizard

## Overview

Transforms the categorization system from rigid category+subcategory pairs to flexible path-based destinations with organic taxonomy growth.

## Problem Statement

**Current System:**
- Requires BOTH category AND subcategory (atomic pair)
- Cannot place files at parent level (e.g., "Audio/" vs "Audio/DAWs/")
- Static taxonomy - must pre-define all categories before sorting

**User Request:**
- "Audio", "Audio/DAWs", and "Audio/Sounds" should all be valid, distinct destinations
- "Audio" can contain general audio files that aren't DAWs or Sounds
- Dynamic category creation during sorting when AI uncertain or no match found
- Taxonomy evolves naturally over time

## Solution Architecture

### Phase 1: Path-Based Categorization (Core)

**Key Changes:**

1. **Tree Path Extraction**
   ```cpp
   // Before: flatten_to_legacy()
   categories=[Audio], subcategories=[DAWs, Sounds]
   
   // After: get_all_paths()
   paths=["Audio", "Audio/DAWs", "Audio/Sounds"]
   ```

2. **Validation Update**
   ```cpp
   // Allow empty subcategory for parent placement
   LabelValidationResult validate_path(const std::string& path) {
       if (path.empty()) return {false, "Path is empty"};
       // Validate each segment independently
       auto segments = split_path(path);
       for (const auto& seg : segments) {
           if (!is_valid_segment(seg)) {
               return {false, fmt::format("Invalid segment: {}", seg)};
           }
       }
       return {true, {}};
   }
   ```

3. **AI Prompt Changes**
   ```
   // Old prompt:
   "Pick category from list A and subcategory from list B"
   
   // New prompt:
   "Pick ONE destination path from this list:
   1) Audio
   2) Audio/DAWs
   3) Audio/Sounds
   4) Documents
   5) Documents/Work
   ..."
   ```

4. **Resolution Logic**
   ```cpp
   DatabaseManager::ResolvedCategory resolve_path(const std::string& path) {
       // path = "Audio/DAWs"
       auto segments = split_path(path);
       // segments = ["Audio", "DAWs"]
       
       if (segments.size() == 1) {
           // Parent-only placement
           return {id, segments[0], ""};
       }
       // Multi-level placement
       return {id, segments[0], join(segments.begin()+1, segments.end(), "/")};
   }
   ```

### Phase 2: Dynamic Category Wizard

**Activation:**
- Settings checkbox: "Enable category suggestion wizard"
- Triggered when:
  1. AI returns "UNCERTAIN"
  2. AI picks generic category ("Uncategorized", "Other", etc.)
  3. Whitelist enforcement finds no valid path

**Wizard Dialog Flow:**

```
┌─────────────────────────────────────────────┐
│ Create New Category Suggestion             │
├─────────────────────────────────────────────┤
│ File: vacation_photo_2024.jpg              │
│ [Preview thumbnail/text]                    │
│                                             │
│ AI suggested: Photos                        │
│                                             │
│ ○ Use existing parent: Photos              │
│ ● Create new subcategory under Photos:     │
│   ┌──────────────────────────────────────┐ │
│   │ Vacation                             │ │
│   └──────────────────────────────────────┘ │
│ ○ Skip this file                           │
│                                             │
│ ☑ Apply to similar files in this batch    │
│                                             │
│ [Create & Continue]  [Cancel]              │
└─────────────────────────────────────────────┘
```

**Implementation:**

1. **New Dialog Class**
   ```cpp
   class CategorySuggestionWizard : public QDialog {
   public:
       enum Result { CreateNew, UseParent, Skip };
       
       CategorySuggestionWizard(
           const FileEntry& file,
           const std::string& suggested_parent,
           const std::vector<std::string>& existing_paths,
           QWidget* parent = nullptr
       );
       
       Result get_result() const;
       std::string get_path() const;  // e.g., "Photos/Vacation"
       bool apply_to_similar() const;
   };
   ```

2. **Integration Point**
   ```cpp
   // In categorize_single_entry()
   if (category_subcategory.find("UNCERTAIN") == 0) {
       if (settings.get_enable_category_wizard()) {
           auto result = show_category_wizard(entry);
           if (result.action == WizardResult::CreateNew) {
               // Add to whitelist dynamically
               add_path_to_whitelist(result.path);
               return resolve_path(result.path);
           } else if (result.action == WizardResult::UseParent) {
               return resolve_path(result.parent_path);
           }
       }
       return DatabaseManager::ResolvedCategory{-1, "", ""};
   }
   ```

3. **Dynamic Whitelist Update**
   ```cpp
   void CategorizationService::add_path_to_whitelist(const std::string& path) {
       auto segments = split_path(path);
       
       // Update whitelist entry
       auto entry = whitelist_store.get(whitelist_store.default_name());
       
       // Add to tree structure
       CategoryNode* current = &entry.tree_root;
       for (const auto& segment : segments) {
           auto it = std::find_if(current->children.begin(),
                                 current->children.end(),
                                 [&](const auto& n) { return n.name == segment; });
           if (it == current->children.end()) {
               current->children.push_back({segment, {}});
               it = current->children.end() - 1;
           }
           current = &(*it);
       }
       
       whitelist_store.set(whitelist_store.default_name(), entry);
       whitelist_store.save();
       
       // Refresh cached path list
       refresh_allowed_paths();
   }
   ```

### Phase 3: Natural Evolution Features

**Usage Tracking:**
```sql
CREATE TABLE category_usage (
    path TEXT PRIMARY KEY,
    use_count INTEGER DEFAULT 0,
    last_used TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**Consolidation Suggestions:**
```cpp
// Suggest merging rarely-used categories
auto rare_categories = db.query(
    "SELECT path FROM category_usage WHERE use_count < 3 AND "
    "last_used < datetime('now', '-30 days')"
);
```

**Export/Import:**
```cpp
// Export refined taxonomy
void export_whitelist(const std::string& filename, ExportFormat format);
// Formats: JSON, CSV, YAML

// Import from other projects
void import_whitelist(const std::string& filename, MergeStrategy strategy);
// Strategies: Replace, Merge, Append
```

## API Changes

### WhitelistEntry Extensions

```cpp
struct WhitelistEntry {
    // ... existing fields ...
    
    // New methods for path-based access
    std::vector<std::string> get_all_paths() const;
    void add_path(const std::string& path);
    void remove_path(const std::string& path);
    bool has_path(const std::string& path) const;
    
    // Backwards compatibility
    void migrate_to_path_based();  // Convert old category+subcategory
};
```

### Settings Extensions

```cpp
class Settings {
    // ... existing methods ...
    
    bool get_enable_category_wizard() const;
    void set_enable_category_wizard(bool enabled);
    
    std::vector<std::string> get_allowed_paths() const;
    void set_allowed_paths(const std::vector<std::string>& paths);
};
```

### CategorizationService Extensions

```cpp
class CategorizationService {
    // ... existing methods ...
    
    // New path-based methods
    DatabaseManager::ResolvedCategory resolve_path(const std::string& path);
    void add_path_to_whitelist(const std::string& path);
    std::vector<std::string> get_active_paths() const;
    
private:
    void refresh_allowed_paths();
    CategoryWizardResult show_category_wizard(const FileEntry& entry);
};
```

## Migration Strategy

**Backwards Compatibility:**

1. **Detect Old Format:**
   ```cpp
   bool WhitelistEntry::is_legacy_format() const {
       return !use_hierarchical && 
              !categories.empty() && 
              !subcategories.empty();
   }
   ```

2. **Auto-Migrate:**
   ```cpp
   void WhitelistEntry::migrate_to_path_based() {
       if (!is_legacy_format()) return;
       
       // Convert category+subcategory pairs to paths
       for (const auto& cat : categories) {
           add_path(cat);  // Parent-level
           for (const auto& sub : subcategories) {
               add_path(cat + "/" + sub);  // Child-level
           }
       }
       
       use_hierarchical = true;
       flatten_to_legacy();  // Update flat lists
   }
   ```

3. **Prompt User:**
   ```
   "Your whitelist uses the old category+subcategory format.
    Would you like to migrate to the new path-based system?
    
    This will allow files to be placed at parent levels
    (e.g., 'Audio/' vs 'Audio/DAWs/').
    
    [Migrate Now] [Keep Old Format] [Learn More]"
   ```

## UI Changes

**Settings Dialog:**
```
☑ Enable path-based categorization
   ↳ Allows files at any tree level (Audio, Audio/DAWs, etc.)

☑ Enable category suggestion wizard (requires path-based)
   ↳ Prompt to create new categories during sorting
```

**Whitelist Editor:**
- Add "Path View" toggle button
- Show full paths in tree: "Audio", "Audio/DAWs" (both selectable)
- Right-click context menu: "Copy full path"

## Performance Considerations

**Path List Caching:**
```cpp
class CategorizationService {
    mutable std::vector<std::string> cached_paths_;
    mutable bool paths_dirty_ = true;
    
    const std::vector<std::string>& get_allowed_paths() const {
        if (paths_dirty_) {
            cached_paths_ = extract_paths_from_whitelist();
            paths_dirty_ = false;
        }
        return cached_paths_;
    }
};
```

**Wizard Performance:**
- Async file preview loading
- Thumbnail cache (SQLite BLOB)
- Limit preview size (first 1KB of text, 200x200 thumbnail)

## Testing Strategy

**Unit Tests:**
```cpp
TEST(PathBasedCategorizationTest, ParentLevelPlacement) {
    auto result = service.resolve_path("Audio");
    EXPECT_EQ(result.category, "Audio");
    EXPECT_EQ(result.subcategory, "");
}

TEST(PathBasedCategorizationTest, MultiLevelPath) {
    auto result = service.resolve_path("Audio/DAWs/Plugins");
    EXPECT_EQ(result.category, "Audio");
    EXPECT_EQ(result.subcategory, "DAWs/Plugins");
}

TEST(WhitelistEntryTest, PathExtraction) {
    WhitelistEntry entry = create_test_tree();
    auto paths = entry.get_all_paths();
    EXPECT_THAT(paths, Contains("Audio"));
    EXPECT_THAT(paths, Contains("Audio/DAWs"));
    EXPECT_THAT(paths, Contains("Audio/Sounds"));
}
```

**Integration Tests:**
```cpp
TEST(CategoryWizardTest, DynamicCreation) {
    enable_wizard_mode();
    auto files = {FileEntry{"test.wav"}};
    
    // Simulate wizard interaction
    mock_wizard_response("Audio/Samples");
    
    auto results = service.categorize_entries(files, ...);
    
    EXPECT_EQ(results[0].category, "Audio");
    EXPECT_EQ(results[0].subcategory, "Samples");
    
    // Verify added to whitelist
    auto paths = get_whitelist_paths();
    EXPECT_THAT(paths, Contains("Audio/Samples"));
}
```

## Documentation

**User Guide Section:**
```markdown
### Path-Based Categorization

Files can now be placed at any level of your category tree:

- **Parent Level**: General files (e.g., "Audio/")
- **Child Level**: Specific files (e.g., "Audio/DAWs/")
- **Deep Nesting**: Highly specific (e.g., "Audio/DAWs/VST-Plugins/")

### Category Suggestion Wizard

When the AI is uncertain, you'll be prompted to create new categories:

1. Review the file preview
2. Choose action:
   - Create new subcategory
   - Use existing parent
   - Skip file
3. Apply to similar files in batch

Your taxonomy grows organically as you sort!
```

## Timeline

**Week 1: Core Path-Based System**
- Modify `WhitelistEntry` path extraction
- Update `validate_labels()` for path validation
- Change AI prompt generation
- Implement `resolve_path()`
- Unit tests

**Week 2: Dynamic Wizard**
- Create `CategorySuggestionWizard` dialog
- File preview component
- Integrate with categorization flow
- Settings toggle
- Integration tests

**Week 3: Evolution Features**
- Usage tracking
- Consolidation suggestions
- Export/Import
- Documentation

**Week 4: Polish & Testing**
- Migration assistant
- Performance optimization
- User testing
- Bug fixes

## Open Questions

1. **Wizard Appearance Frequency**: How often should wizard appear for similar files?
   - Suggested: Learn from first 3 similar files, then auto-apply
   
2. **Path Separator**: Use "/" or "\" on Windows?
   - Suggested: Always "/" internally, display native on UI
   
3. **Maximum Depth**: Limit tree depth?
   - Suggested: Warning at depth 5, hard limit at 10

4. **Batch Learning**: Should wizard suggestions auto-apply to remaining files?
   - Suggested: Optional checkbox, default ON

## Success Metrics

- ✅ Users can place files at any tree level
- ✅ Wizard reduces "Uncategorized" files by 80%
- ✅ Taxonomy evolution tracked and visualizable
- ✅ Zero breaking changes for existing users
- ✅ Performance impact < 5% overhead
