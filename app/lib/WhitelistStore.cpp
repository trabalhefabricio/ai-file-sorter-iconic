#include "WhitelistStore.hpp"
#include "Logger.hpp"
#include "Settings.hpp"

#include <QSettings>
#include <algorithm>
#include <set>

namespace {
// Changed from comma to semicolon as the primary separator
std::vector<std::string> split_csv(const QString& value) {
    std::vector<std::string> out;
    // First try semicolon (new format)
    if (value.contains(';')) {
        const auto parts = value.split(";");
        for (const auto& part : parts) {
            QString trimmed = part.trimmed();
            if (!trimmed.isEmpty()) {
                out.emplace_back(trimmed.toStdString());
            }
        }
    } else {
        // Fall back to comma for backward compatibility
        const auto parts = value.split(",");
        for (const auto& part : parts) {
            QString trimmed = part.trimmed();
            if (!trimmed.isEmpty()) {
                out.emplace_back(trimmed.toStdString());
            }
        }
    }
    return out;
}

QString join_csv(const std::vector<std::string>& values) {
    QStringList list;
    for (const auto& v : values) {
        list << QString::fromStdString(v);
    }
    // Use semicolon as primary separator now
    return list.join("; ");
}
}

WhitelistStore::WhitelistStore(std::string config_dir)
    : file_path_(std::move(config_dir) + "/whitelists.ini") {}

bool WhitelistStore::load()
{
    entries_.clear();
    QSettings settings(QString::fromStdString(file_path_), QSettings::IniFormat);
    const QStringList groups = settings.childGroups();
    for (const auto& group : groups) {
        settings.beginGroup(group);
        const auto cats = split_csv(settings.value("Categories").toString());
        const auto subs = split_csv(settings.value("Subcategories").toString());
        const auto context = settings.value("Context", "").toString().toStdString();
        const bool advanced = settings.value("AdvancedSubcategories", false).toBool();
        const bool hierarchical = settings.value("UseHierarchical", false).toBool();
        
        WhitelistEntry entry;
        entry.categories = cats;
        entry.subcategories = subs;
        entry.context = context;
        entry.enable_advanced_subcategories = advanced;
        entry.use_hierarchical = hierarchical;
        
        // Load hierarchical structure if present
        if (hierarchical) {
            for (const auto& category : cats) {
                QString key = QString("Subcategories_%1").arg(QString::fromStdString(category));
                auto cat_subs = split_csv(settings.value(key).toString());
                entry.category_subcategory_map[category] = cat_subs;
            }
        }
        
        settings.endGroup();
        if (!cats.empty() || !subs.empty()) {
            entries_[group.toStdString()] = entry;
        }
    }
    if (entries_.empty()) {
        ensure_default_from_legacy({}, {});
        save();
    }
    return true;
}

bool WhitelistStore::save() const
{
    QSettings settings(QString::fromStdString(file_path_), QSettings::IniFormat);
    settings.clear();
    for (const auto& pair : entries_) {
        settings.beginGroup(QString::fromStdString(pair.first));
        settings.setValue("Categories", join_csv(pair.second.categories));
        settings.setValue("Subcategories", join_csv(pair.second.subcategories));
        settings.setValue("Context", QString::fromStdString(pair.second.context));
        settings.setValue("AdvancedSubcategories", pair.second.enable_advanced_subcategories);
        settings.setValue("UseHierarchical", pair.second.use_hierarchical);
        
        // Save hierarchical structure if present
        if (pair.second.use_hierarchical) {
            for (const auto& [category, subs] : pair.second.category_subcategory_map) {
                QString key = QString("Subcategories_%1").arg(QString::fromStdString(category));
                settings.setValue(key, join_csv(subs));
            }
        }
        
        settings.endGroup();
    }
    settings.sync();
    return settings.status() == QSettings::NoError;
}

std::vector<std::string> WhitelistStore::list_names() const
{
    std::vector<std::string> names;
    names.reserve(entries_.size());
    for (const auto& entry : entries_) {
        names.push_back(entry.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// WhitelistEntry helper methods
std::vector<CategoryNode> WhitelistEntry::to_tree() const {
    std::vector<CategoryNode> nodes;
    
    if (use_hierarchical) {
        // Use hierarchical structure - convert to recursive tree
        for (const auto& [category, subs] : category_subcategory_map) {
            CategoryNode node;
            node.name = category;
            // Convert flat subcategories to child nodes
            for (const auto& sub : subs) {
                CategoryNode child;
                child.name = sub;
                node.children.push_back(child);
            }
            nodes.push_back(node);
        }
    } else {
        // Convert flat structure to tree
        for (const auto& category : categories) {
            CategoryNode node;
            node.name = category;
            // In flat mode, all subcategories are shared - convert to children
            for (const auto& sub : subcategories) {
                CategoryNode child;
                child.name = sub;
                node.children.push_back(child);
            }
            nodes.push_back(node);
        }
    }
    
    return nodes;
}

void WhitelistEntry::from_tree(const std::vector<CategoryNode>& nodes) {
    use_hierarchical = true;
    category_subcategory_map.clear();
    categories.clear();
    subcategories.clear();
    
    for (const auto& node : nodes) {
        categories.push_back(node.name);  // Add to flat list
        
        // Extract direct children as subcategories
        std::vector<std::string> subs;
        for (const auto& child : node.children) {
            subs.push_back(child.name);
            // Note: We're flattening here - deeper nesting will be preserved in future
        }
        category_subcategory_map[node.name] = subs;
    }
}

void WhitelistEntry::flatten_to_legacy() {
    if (!use_hierarchical) {
        return;  // Already flat
    }
    
    // Flatten hierarchical structure to legacy format
    categories.clear();
    subcategories.clear();
    std::set<std::string> unique_subs;
    
    for (const auto& [category, subs] : category_subcategory_map) {
        categories.push_back(category);
        for (const auto& sub : subs) {
            unique_subs.insert(sub);
        }
    }
    
    subcategories.assign(unique_subs.begin(), unique_subs.end());
}

std::optional<WhitelistEntry> WhitelistStore::get(const std::string& name) const
{
    if (auto it = entries_.find(name); it != entries_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void WhitelistStore::set(const std::string& name, WhitelistEntry entry)
{
    entries_[name] = std::move(entry);
}

void WhitelistStore::remove(const std::string& name)
{
    entries_.erase(name);
}

void WhitelistStore::ensure_default_from_legacy(const std::vector<std::string>& cats,
                                                const std::vector<std::string>& subs)
{
    if (!entries_.empty()) {
        return;
    }
    std::vector<std::string> use_cats = cats;
    std::vector<std::string> use_subs = subs;
    if (use_cats.empty()) {
        use_cats = {
            "Archives", "Backups", "Books", "Configs", "Data Exports",
            "Development", "Documents", "Drivers", "Ebooks", "Firmware",
            "Guides", "Images", "Installers", "Licenses", "Manuals",
            "Music", "Operating Systems", "Presentations", "Software", "Spreadsheets", "System",
            "Temporary", "Videos"
        };
    }
    if (use_subs.empty()) {
        use_subs = {};
    }
    WhitelistEntry entry;
    entry.categories = use_cats;
    entry.subcategories = use_subs;
    entry.context = "";
    entry.enable_advanced_subcategories = false;
    entries_[default_name_] = entry;
}

void WhitelistStore::initialize_from_settings(Settings& settings)
{
    load();
    ensure_default_from_legacy(settings.get_allowed_categories(),
                               settings.get_allowed_subcategories());
    save();

    if (settings.get_active_whitelist().empty()) {
        settings.set_active_whitelist(default_name_);
    }

    auto active = settings.get_active_whitelist();
    if (auto entry = get(active)) {
        settings.set_allowed_categories(entry->categories);
        settings.set_allowed_subcategories(entry->subcategories);
    }
}

// Dynamic path addition for wizard mode
bool WhitelistStore::add_path_to_entry(const std::string& entry_name, const std::string& path)
{
    auto it = entries_.find(entry_name);
    if (it == entries_.end()) {
        // Entry doesn't exist, create it
        WhitelistEntry new_entry;
        new_entry.use_hierarchical = true;
        entries_[entry_name] = new_entry;
        it = entries_.find(entry_name);
    }

    auto& entry = it->second;
    
    // Parse path (e.g., "Audio/DAWs/FL Studio" -> ["Audio", "DAWs", "FL Studio"])
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }

    if (parts.empty()) {
        return false;
    }

    // Build hierarchical structure
    if (!entry.use_hierarchical) {
        // Convert to hierarchical if needed
        entry.use_hierarchical = true;
        entry.category_subcategory_map.clear();
    }

    // For now, treat first part as category and rest as nested subcategories
    // This is a simplified approach - full recursive support in future commits
    const std::string& category = parts[0];
    
    // Add category if it doesn't exist
    if (std::find(entry.categories.begin(), entry.categories.end(), category) == entry.categories.end()) {
        entry.categories.push_back(category);
    }

    // Add all parts as subcategories
    for (size_t i = 1; i < parts.size(); ++i) {
        if (std::find(entry.subcategories.begin(), entry.subcategories.end(), parts[i]) == entry.subcategories.end()) {
            entry.subcategories.push_back(parts[i]);
        }
    }

    // Update hierarchical map
    auto& subs = entry.category_subcategory_map[category];
    for (size_t i = 1; i < parts.size(); ++i) {
        if (std::find(subs.begin(), subs.end(), parts[i]) == subs.end()) {
            subs.push_back(parts[i]);
        }
    }

    return true;
}

std::vector<std::string> WhitelistStore::get_all_paths_from_entry(const std::string& entry_name) const
{
    std::vector<std::string> paths;
    
    auto it = entries_.find(entry_name);
    if (it == entries_.end()) {
        return paths;
    }

    const auto& entry = it->second;
    
    // Return all categories
    for (const auto& cat : entry.categories) {
        paths.push_back(cat);
    }

    // If hierarchical, add category/subcategory combinations
    if (entry.use_hierarchical) {
        for (const auto& [category, subs] : entry.category_subcategory_map) {
            for (const auto& sub : subs) {
                paths.push_back(category + "/" + sub);
            }
        }
    }

    return paths;
}
