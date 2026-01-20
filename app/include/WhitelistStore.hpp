#ifndef WHITELIST_STORE_HPP
#define WHITELIST_STORE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <optional>

class Settings;

// Hierarchical category structure for tree-based whitelist editor
// Now supports recursive nesting (sub-subcategories and beyond)
struct CategoryNode {
    std::string name;
    std::vector<CategoryNode> children;  // Recursive: can contain more CategoryNodes
    
    // Helper to check if this is a leaf node
    bool is_leaf() const { return children.empty(); }
    
    // Helper to get depth
    int depth() const {
        if (is_leaf()) return 0;
        int max_depth = 0;
        for (const auto& child : children) {
            max_depth = std::max(max_depth, child.depth());
        }
        return max_depth + 1;
    }
};

struct WhitelistEntry {
    // Legacy flat structure (for backward compatibility)
    std::vector<std::string> categories;
    std::vector<std::string> subcategories;
    
    // New hierarchical structure (preferred)
    std::map<std::string, std::vector<std::string>> category_subcategory_map;
    
    std::string context;  // User-provided context for better categorization
    bool enable_advanced_subcategories{false};  // Generate subcategories dynamically
    bool use_hierarchical{false};  // True if using category_subcategory_map
    
    // Helper methods
    std::vector<CategoryNode> to_tree() const;
    void from_tree(const std::vector<CategoryNode>& nodes);
    void flatten_to_legacy();  // Convert hierarchical to flat for AI prompt
};

class WhitelistStore {
public:
    explicit WhitelistStore(std::string config_dir);

    bool load();
    bool save() const;

    std::vector<std::string> list_names() const;
    std::optional<WhitelistEntry> get(const std::string& name) const;
    void set(const std::string& name, WhitelistEntry entry);
    void remove(const std::string& name);
    bool empty() const { return entries_.empty(); }

    // Migration helper
    void ensure_default_from_legacy(const std::vector<std::string>& cats,
                                    const std::vector<std::string>& subs);
    void initialize_from_settings(Settings& settings);

    // Dynamic path addition for wizard mode
    bool add_path_to_entry(const std::string& entry_name, const std::string& path);
    std::vector<std::string> get_all_paths_from_entry(const std::string& entry_name) const;

    std::string default_name() const { return default_name_; }

private:
    std::string file_path_;
    std::unordered_map<std::string, WhitelistEntry> entries_;
    std::string default_name_ = "Default";
};

#endif
