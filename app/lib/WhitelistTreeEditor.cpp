#include "WhitelistTreeEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QKeyEvent>
#include <QShortcut>

WhitelistTreeEditor::WhitelistTreeEditor(const QString& name, WhitelistEntry& entry, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit Whitelist - Tree View"));
    resize(700, 650);
    setup_ui();
    
    name_edit_->setText(name);
    context_edit_->setPlainText(QString::fromStdString(entry.context));
    advanced_checkbox_->setChecked(entry.enable_advanced_subcategories);
    
    // Set mode based on entry
    if (entry.use_hierarchical) {
        hierarchical_mode_radio_->setChecked(true);
    } else {
        shared_mode_radio_->setChecked(true);
        // Store shared subcategories
        for (const auto& sub : entry.subcategories) {
            shared_subcategories_ << QString::fromStdString(sub);
        }
    }
    
    populate_tree(entry);
    update_mode_ui();
}

void WhitelistTreeEditor::setup_ui()
{
    auto* main_layout = new QVBoxLayout(this);
    
    // Name field
    main_layout->addWidget(new QLabel(tr("Whitelist Name:"), this));
    name_edit_ = new QLineEdit(this);
    main_layout->addWidget(name_edit_);
    
    // Mode selection
    auto* mode_group_box = new QGroupBox(tr("Subcategory Mode"), this);
    auto* mode_layout = new QVBoxLayout(mode_group_box);
    
    mode_group_ = new QButtonGroup(this);
    
    hierarchical_mode_radio_ = new QRadioButton(tr("Hierarchical (each category has its own subcategories)"), this);
    hierarchical_mode_radio_->setToolTip(tr("Each category can have different subcategories"));
    mode_group_->addButton(hierarchical_mode_radio_, 0);
    mode_layout->addWidget(hierarchical_mode_radio_);
    
    shared_mode_radio_ = new QRadioButton(tr("Shared (all categories share the same subcategories)"), this);
    shared_mode_radio_->setToolTip(tr("All categories use the same set of subcategories (classic mode)"));
    mode_group_->addButton(shared_mode_radio_, 1);
    mode_layout->addWidget(shared_mode_radio_);
    
    main_layout->addWidget(mode_group_box);
    
    // Tree widget with instructions
    auto* tree_label = new QLabel(tr("Categories and Subcategories:"), this);
    main_layout->addWidget(tree_label);
    
    tree_widget_ = new QTreeWidget(this);
    tree_widget_->setHeaderLabels({tr("Category / Subcategory"), tr("Type")});
    tree_widget_->setColumnWidth(0, 400);
    tree_widget_->setEditTriggers(QAbstractItemView::EditTrigger::DoubleClicked);
    tree_widget_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_widget_->setAlternatingRowColors(true);
    
    // Explicitly disable drag-drop to prevent dropEvent crashes on Qt version mismatch
    tree_widget_->setDragEnabled(false);
    tree_widget_->setAcceptDrops(false);
    tree_widget_->setDragDropMode(QAbstractItemView::NoDragDrop);
    
    main_layout->addWidget(tree_widget_);
    
    // Buttons row
    auto* btn_layout = new QHBoxLayout();
    
    add_category_btn_ = new QPushButton(tr("+ Category"), this);
    add_category_btn_->setToolTip(tr("Add a new category at the root level"));
    add_category_btn_->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    btn_layout->addWidget(add_category_btn_);
    
    add_subcategory_btn_ = new QPushButton(tr("+ Child"), this);
    add_subcategory_btn_->setToolTip(tr("Add a child item to the selected item (supports unlimited nesting)"));
    add_subcategory_btn_->setIcon(style()->standardIcon(QStyle::SP_FileLinkIcon));
    add_subcategory_btn_->setEnabled(false);
    btn_layout->addWidget(add_subcategory_btn_);
    
    remove_btn_ = new QPushButton(tr("Remove"), this);
    remove_btn_->setToolTip(tr("Remove selected item"));
    remove_btn_->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    remove_btn_->setEnabled(false);
    btn_layout->addWidget(remove_btn_);
    
    btn_layout->addStretch();
    
    import_btn_ = new QPushButton(tr("Import"), this);
    import_btn_->setToolTip(tr("Import whitelist configuration from JSON file"));
    import_btn_->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    btn_layout->addWidget(import_btn_);
    
    export_btn_ = new QPushButton(tr("Export"), this);
    export_btn_->setToolTip(tr("Export whitelist configuration to JSON file"));
    export_btn_->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    btn_layout->addWidget(export_btn_);
    
    btn_layout->addStretch();
    main_layout->addLayout(btn_layout);
    
    // Shared subcategories section (only visible in shared mode)
    shared_subs_group_ = new QGroupBox(tr("Shared Subcategories"), this);
    auto* shared_layout = new QHBoxLayout(shared_subs_group_);
    
    shared_subs_label_ = new QLabel(tr("(none)"), this);
    shared_subs_label_->setWordWrap(true);
    shared_layout->addWidget(shared_subs_label_, 1);
    
    edit_shared_subs_btn_ = new QPushButton(tr("Edit..."), this);
    edit_shared_subs_btn_->setToolTip(tr("Edit the shared subcategories list"));
    shared_layout->addWidget(edit_shared_subs_btn_);
    
    main_layout->addWidget(shared_subs_group_);
    
    // Context field
    main_layout->addWidget(new QLabel(tr("Context (describe what files are being sorted):"), this));
    context_edit_ = new QTextEdit(this);
    context_edit_->setMaximumHeight(80);
    main_layout->addWidget(context_edit_);
    
    // Advanced checkbox
    advanced_checkbox_ = new QCheckBox(tr("Enable advanced subcategory generation"), this);
    advanced_checkbox_->setToolTip(tr("Generate dynamic subcategories based on categories and files"));
    main_layout->addWidget(advanced_checkbox_);
    
    // Dialog buttons
    auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    main_layout->addWidget(button_box);
    
    // Connect signals
    connect(add_category_btn_, &QPushButton::clicked, this, &WhitelistTreeEditor::on_add_category);
    connect(add_subcategory_btn_, &QPushButton::clicked, this, &WhitelistTreeEditor::on_add_child_to_selected);
    connect(remove_btn_, &QPushButton::clicked, this, &WhitelistTreeEditor::on_remove_item);
    connect(tree_widget_, &QTreeWidget::itemChanged, this, &WhitelistTreeEditor::on_item_changed);
    connect(tree_widget_, &QTreeWidget::itemSelectionChanged, this, &WhitelistTreeEditor::on_selection_changed);
    connect(mode_group_, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), 
            this, &WhitelistTreeEditor::on_mode_changed);
    connect(edit_shared_subs_btn_, &QPushButton::clicked, this, &WhitelistTreeEditor::on_edit_shared_subcategories);
    connect(import_btn_, &QPushButton::clicked, this, &WhitelistTreeEditor::on_import_config);
    connect(export_btn_, &QPushButton::clicked, this, &WhitelistTreeEditor::on_export_config);
    connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void WhitelistTreeEditor::update_mode_ui()
{
    bool is_hierarchical = hierarchical_mode_radio_->isChecked();
    
    // Show/hide shared subcategories section
    shared_subs_group_->setVisible(!is_hierarchical);
    
    // Update subcategory button visibility/enablement
    if (!is_hierarchical) {
        add_subcategory_btn_->setVisible(false);  // Hide in shared mode
        
        // Update shared subcategories label
        if (shared_subcategories_.isEmpty()) {
            shared_subs_label_->setText(tr("(none)"));
        } else {
            shared_subs_label_->setText(shared_subcategories_.join("; "));
        }
    } else {
        add_subcategory_btn_->setVisible(true);  // Show in hierarchical mode
    }
    
    // Update tree to reflect mode
    populate_tree_for_mode();
}

// BUG FIX #2: Added null checks for topLevelItem (line 182 in original)
QStringList WhitelistTreeEditor::get_category_names() const
{
    QStringList categories;
    for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
        auto* item = tree_widget_->topLevelItem(i);
        if (item) {  // NULL CHECK ADDED
            categories << item->text(0);
        }
    }
    return categories;
}

// BUG FIX #2: Added null checks for child items (lines 230-232 in original)
QSet<QString> WhitelistTreeEditor::get_all_subcategories() const
{
    QSet<QString> unique_subs;
    for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
        auto* cat_item = tree_widget_->topLevelItem(i);
        if (cat_item) {  // NULL CHECK ADDED
            for (int j = 0; j < cat_item->childCount(); ++j) {
                auto* child = cat_item->child(j);
                if (child) {  // NULL CHECK ADDED
                    unique_subs.insert(child->text(0));
                }
            }
        }
    }
    return unique_subs;
}

void WhitelistTreeEditor::populate_tree_for_mode()
{
    QStringList categories = get_category_names();
    
    tree_widget_->clear();
    updating_tree_ = true;
    
    // If we have no categories, nothing to repopulate
    if (categories.isEmpty()) {
        updating_tree_ = false;
        return;
    }
    
    bool is_hierarchical = hierarchical_mode_radio_->isChecked();
    
    if (is_hierarchical) {
        // In hierarchical mode, each category shows its own subcategories
        // Keep existing tree structure
    } else {
        // In shared mode, all categories use the same subcategories
        for (const auto& cat : categories) {
            add_category_node(cat, shared_subcategories_);
        }
    }
    
    tree_widget_->expandAll();
    updating_tree_ = false;
}

void WhitelistTreeEditor::on_mode_changed()
{
    bool switching_to_shared = shared_mode_radio_->isChecked();
    
    if (switching_to_shared) {
        // When switching to shared mode, ask if user wants to collect all unique subcategories
        auto reply = QMessageBox::question(this,
                                          tr("Switch to Shared Mode"),
                                          tr("Collect all unique subcategories from categories?\n\n"
                                             "Yes: Merge all subcategories into a shared list\n"
                                             "No: Keep current shared subcategories (if any)"),
                                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        
        if (reply == QMessageBox::Cancel) {
            // Revert mode change
            hierarchical_mode_radio_->setChecked(true);
            return;
        }
        
        if (reply == QMessageBox::Yes) {
            // BUG FIX #2: Use the fixed method with null checks
            auto unique_subs = get_all_subcategories();
            shared_subcategories_ = unique_subs.values();
            shared_subcategories_.sort();
        }
    }
    
    update_mode_ui();
}

void WhitelistTreeEditor::on_edit_shared_subcategories()
{
    // Show a text edit dialog for shared subcategories
    bool ok;
    QString text = QInputDialog::getMultiLineText(this,
                                                  tr("Edit Shared Subcategories"),
                                                  tr("Enter subcategories (one per line or semicolon-separated):"),
                                                  shared_subcategories_.join("\n"),
                                                  &ok);
    
    if (ok) {
        shared_subcategories_.clear();
        
        // Parse input - support both newlines and semicolons
        QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        for (const auto& line : lines) {
            if (line.contains(';')) {
                QStringList parts = line.split(';', Qt::SkipEmptyParts);
                for (const auto& part : parts) {
                    QString trimmed = part.trimmed();
                    if (!trimmed.isEmpty()) {
                        shared_subcategories_ << trimmed;
                    }
                }
            } else {
                QString trimmed = line.trimmed();
                if (!trimmed.isEmpty()) {
                    shared_subcategories_ << trimmed;
                }
            }
        }
        
        // Remove duplicates and sort
        shared_subcategories_.removeDuplicates();
        shared_subcategories_.sort();
        
        update_mode_ui();
    }
}

void WhitelistTreeEditor::populate_tree(const WhitelistEntry& entry)
{
    tree_widget_->clear();
    updating_tree_ = true;
    
    auto nodes = entry.to_tree();
    for (const auto& node : nodes) {
        add_node_recursive(nullptr, node);
    }
    
    tree_widget_->expandAll();
    updating_tree_ = false;
}

void WhitelistTreeEditor::add_category_node(const QString& name, const QStringList& subcategories)
{
    auto* category_item = new QTreeWidgetItem(tree_widget_);
    category_item->setText(0, name);
    category_item->setText(1, tr("Category"));
    category_item->setFlags(category_item->flags() | Qt::ItemIsEditable);
    category_item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    category_item->setData(0, Qt::UserRole, "category");
    
    for (const auto& sub : subcategories) {
        auto* sub_item = new QTreeWidgetItem(category_item);
        sub_item->setText(0, sub);
        sub_item->setText(1, tr("Subcategory"));
        sub_item->setFlags(sub_item->flags() | Qt::ItemIsEditable);
        sub_item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        sub_item->setData(0, Qt::UserRole, "subcategory");
    }
}

WhitelistEntry WhitelistTreeEditor::get_entry() const
{
    WhitelistEntry entry;
    entry.context = context_edit_->toPlainText().toStdString();
    entry.enable_advanced_subcategories = advanced_checkbox_->isChecked();
    
    bool is_hierarchical = hierarchical_mode_radio_->isChecked();
    entry.use_hierarchical = is_hierarchical;
    
    if (is_hierarchical) {
        // Hierarchical mode: recursively extract tree structure
        std::vector<CategoryNode> nodes;
        
        for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
            auto* cat_item = tree_widget_->topLevelItem(i);
            if (cat_item) {  // NULL CHECK
                nodes.push_back(item_to_node(cat_item));
            }
        }
        
        entry.from_tree(nodes);
        entry.flatten_to_legacy();  // Also populate flat structure for compatibility
    } else {
        // Shared mode: all categories use the same subcategories (classic mode)
        entry.categories.clear();
        entry.subcategories.clear();
        
        for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
            auto* cat_item = tree_widget_->topLevelItem(i);
            if (cat_item) {  // NULL CHECK
                entry.categories.push_back(cat_item->text(0).toStdString());
            }
        }
        
        for (const auto& sub : shared_subcategories_) {
            entry.subcategories.push_back(sub.toStdString());
        }
        
        // Clear hierarchical map in shared mode
        entry.category_subcategory_map.clear();
    }
    
    return entry;
}

void WhitelistTreeEditor::on_add_category()
{
    bool ok;
    QString name = QInputDialog::getText(this,
                                         tr("Add Category"),
                                         tr("Category name:"),
                                         QLineEdit::Normal,
                                         QString(),
                                         &ok);
    if (ok && !name.isEmpty()) {
        add_category_node(name, QStringList());
        tree_widget_->expandAll();
    }
}

void WhitelistTreeEditor::on_add_subcategory()
{
    // Legacy method - now delegates to on_add_child_to_selected
    on_add_child_to_selected();
}

void WhitelistTreeEditor::on_add_child_to_selected()
{
    auto* selected_item = tree_widget_->currentItem();
    if (!selected_item) {
        QMessageBox::warning(this, tr("No Item Selected"),
                           tr("Please select an item to add a child to."));
        return;
    }
    
    bool ok;
    QString name = QInputDialog::getText(this,
                                         tr("Add Child Item"),
                                         tr("Child item name:"),
                                         QLineEdit::Normal,
                                         QString(),
                                         &ok);
    if (ok && !name.isEmpty()) {
        updating_tree_ = true;
        auto* child_item = new QTreeWidgetItem(selected_item);
        child_item->setText(0, name);
        
        // Determine type based on depth
        int depth = 0;
        QTreeWidgetItem* temp = selected_item;
        while (temp->parent()) {
            depth++;
            temp = temp->parent();
        }
        
        if (depth == 0) {
            child_item->setText(1, tr("Subcategory"));
        } else if (depth == 1) {
            child_item->setText(1, tr("Sub-subcategory"));
        } else {
            child_item->setText(1, tr("Level %1").arg(depth + 2));
        }
        
        child_item->setFlags(child_item->flags() | Qt::ItemIsEditable);
        child_item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        child_item->setData(0, Qt::UserRole, QString("level%1").arg(depth + 1));
        selected_item->setExpanded(true);
        updating_tree_ = false;
    }
}

// BUG FIX #10: Added null checks for child items (lines 429-430 in original)
CategoryNode WhitelistTreeEditor::item_to_node(QTreeWidgetItem* item) const
{
    CategoryNode node;
    node.name = item->text(0).toStdString();
    
    // Recursively convert children
    for (int i = 0; i < item->childCount(); ++i) {
        auto* child = item->child(i);
        if (child) {  // NULL CHECK ADDED
            node.children.push_back(item_to_node(child));
        }
    }
    
    return node;
}

void WhitelistTreeEditor::add_node_recursive(QTreeWidgetItem* parent, const CategoryNode& node)
{
    QTreeWidgetItem* item;
    if (parent) {
        item = new QTreeWidgetItem(parent);
    } else {
        item = new QTreeWidgetItem(tree_widget_);
    }
    
    item->setText(0, QString::fromStdString(node.name));
    
    // Determine depth for type label
    int depth = 0;
    QTreeWidgetItem* temp = parent;
    while (temp) {
        depth++;
        temp = temp->parent();
    }
    
    if (depth == 0) {
        item->setText(1, tr("Category"));
        item->setData(0, Qt::UserRole, "category");
        item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    } else if (depth == 1) {
        item->setText(1, tr("Subcategory"));
        item->setData(0, Qt::UserRole, "subcategory");
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    } else if (depth == 2) {
        item->setText(1, tr("Sub-subcategory"));
        item->setData(0, Qt::UserRole, "subsubcategory");
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    } else {
        item->setText(1, tr("Level %1").arg(depth + 1));
        item->setData(0, Qt::UserRole, QString("level%1").arg(depth));
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    }
    
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    
    // Recursively add children
    for (const auto& child : node.children) {
        add_node_recursive(item, child);
    }
}

// BUG FIX #5: Fixed Qt memory management (line 498 in original)
void WhitelistTreeEditor::on_remove_item()
{
    auto* item = tree_widget_->currentItem();
    if (!item) return;
    
    QString type = item->data(0, Qt::UserRole).toString();
    QString name = item->text(0);
    
    auto reply = QMessageBox::question(this,
                                       tr("Confirm Removal"),
                                       tr("Remove '%1' (%2)?").arg(name, type),
                                       QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (item->parent()) {
            // Child item - remove and delete
            delete item->parent()->takeChild(item->parent()->indexOfChild(item));
        } else {
            // Top-level item - remove and delete
            // FIXED: Both child and top-level items need explicit delete
            delete tree_widget_->takeTopLevelItem(tree_widget_->indexOfTopLevelItem(item));
        }
    }
}

void WhitelistTreeEditor::on_item_changed(QTreeWidgetItem* item, int column)
{
    if (updating_tree_ || column != 0) return;
    
    // Validate the name
    QString name = item->text(0).trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Name"),
                           tr("Name cannot be empty."));
        updating_tree_ = true;
        item->setText(0, tr("Unnamed"));
        updating_tree_ = false;
    }
}

void WhitelistTreeEditor::on_selection_changed()
{
    auto* item = tree_widget_->currentItem();
    
    if (!item) {
        add_subcategory_btn_->setEnabled(false);
        remove_btn_->setEnabled(false);
        return;
    }
    
    // In hierarchical mode, allow adding children to any item
    // In shared mode, only allow at root level (handled by mode UI update)
    bool is_hierarchical = hierarchical_mode_radio_->isChecked();
    add_subcategory_btn_->setEnabled(is_hierarchical);  // Enable for any item in hierarchical mode
    remove_btn_->setEnabled(true);
}

QTreeWidgetItem* WhitelistTreeEditor::get_selected_category_node()
{
    auto* item = tree_widget_->currentItem();
    if (!item) return nullptr;
    
    QString type = item->data(0, Qt::UserRole).toString();
    if (type == "category") {
        return item;
    } else if (type == "subcategory" && item->parent()) {
        return item->parent();
    }
    
    return nullptr;
}

void WhitelistTreeEditor::enable_drag_drop()
{
    // Note: Drag-drop is intentionally disabled to prevent crashes
    // See setup_ui() where we set DragDropMode to NoDragDrop
}

void WhitelistTreeEditor::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Delete:
            if (tree_widget_->currentItem()) {
                on_remove_item();
            }
            break;
        case Qt::Key_Insert:
            if (tree_widget_->currentItem() && hierarchical_mode_radio_->isChecked()) {
                on_add_child_to_selected();
            } else {
                on_add_category();
            }
            break;
        case Qt::Key_N:
            if (event->modifiers() & Qt::ControlModifier) {
                if (tree_widget_->currentItem() && hierarchical_mode_radio_->isChecked()) {
                    on_add_child_to_selected();
                } else {
                    on_add_category();
                }
            } else {
                QDialog::keyPressEvent(event);
            }
            break;
        case Qt::Key_F2:
            if (tree_widget_->currentItem()) {
                tree_widget_->editItem(tree_widget_->currentItem(), 0);
            }
            break;
        default:
            QDialog::keyPressEvent(event);
    }
}

void WhitelistTreeEditor::on_import_config()
{
    QString file_name = QFileDialog::getOpenFileName(this,
        tr("Import Whitelist Configuration"),
        QString(),
        tr("JSON Files (*.json);;All Files (*)"));
    
    if (file_name.isEmpty()) {
        return;
    }
    
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Import Error"),
            tr("Failed to open file: %1").arg(file.errorString()));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parse_error);
    
    if (parse_error.error != QJsonParseError::NoError) {
        QMessageBox::critical(this, tr("Import Error"),
            tr("Failed to parse JSON: %1").arg(parse_error.errorString()));
        return;
    }
    
    if (!doc.isObject()) {
        QMessageBox::critical(this, tr("Import Error"),
            tr("Invalid JSON format: expected object"));
        return;
    }
    
    QJsonObject root = doc.object();
    
    // Import name
    if (root.contains("name")) {
        name_edit_->setText(root["name"].toString());
    }
    
    // Import context
    if (root.contains("context")) {
        context_edit_->setPlainText(root["context"].toString());
    }
    
    // Import advanced subcategories flag
    if (root.contains("enable_advanced_subcategories")) {
        advanced_checkbox_->setChecked(root["enable_advanced_subcategories"].toBool());
    }
    
    // Import mode
    bool is_hierarchical = root.value("use_hierarchical", false).toBool();
    if (is_hierarchical) {
        hierarchical_mode_radio_->setChecked(true);
    } else {
        shared_mode_radio_->setChecked(true);
    }
    
    // Import tree structure
    tree_widget_->clear();
    updating_tree_ = true;
    
    if (is_hierarchical && root.contains("tree")) {
        QJsonArray tree_array = root["tree"].toArray();
        for (const QJsonValue& node_value : tree_array) {
            if (node_value.isObject()) {
                import_node_recursive(nullptr, node_value.toObject());
            }
        }
    } else if (!is_hierarchical) {
        // Import shared mode
        if (root.contains("categories")) {
            QJsonArray cats = root["categories"].toArray();
            for (const QJsonValue& cat_value : cats) {
                QString cat_name = cat_value.toString();
                if (!cat_name.isEmpty()) {
                    add_category_node(cat_name, QStringList());
                }
            }
        }
        
        if (root.contains("subcategories")) {
            shared_subcategories_.clear();
            QJsonArray subs = root["subcategories"].toArray();
            for (const QJsonValue& sub_value : subs) {
                QString sub_name = sub_value.toString();
                if (!sub_name.isEmpty()) {
                    shared_subcategories_ << sub_name;
                }
            }
        }
    }
    
    tree_widget_->expandAll();
    updating_tree_ = false;
    update_mode_ui();
    
    QMessageBox::information(this, tr("Import Successful"),
        tr("Whitelist configuration imported successfully."));
}

void WhitelistTreeEditor::on_export_config()
{
    QString file_name = QFileDialog::getSaveFileName(this,
        tr("Export Whitelist Configuration"),
        "whitelist_config.json",
        tr("JSON Files (*.json);;All Files (*)"));
    
    if (file_name.isEmpty()) {
        return;
    }
    
    QJsonObject root;
    
    // Export name
    root["name"] = name_edit_->text();
    
    // Export context
    root["context"] = context_edit_->toPlainText();
    
    // Export advanced subcategories flag
    root["enable_advanced_subcategories"] = advanced_checkbox_->isChecked();
    
    // Export mode
    bool is_hierarchical = hierarchical_mode_radio_->isChecked();
    root["use_hierarchical"] = is_hierarchical;
    
    if (is_hierarchical) {
        // Export hierarchical tree
        QJsonArray tree_array;
        for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
            auto* item = tree_widget_->topLevelItem(i);
            if (item) {
                tree_array.append(export_node_recursive(item));
            }
        }
        root["tree"] = tree_array;
    } else {
        // Export shared mode
        QJsonArray cats;
        for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
            auto* item = tree_widget_->topLevelItem(i);
            if (item) {
                cats.append(item->text(0));
            }
        }
        root["categories"] = cats;
        
        QJsonArray subs;
        for (const QString& sub : shared_subcategories_) {
            subs.append(sub);
        }
        root["subcategories"] = subs;
    }
    
    QJsonDocument doc(root);
    
    QFile file(file_name);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Error"),
            tr("Failed to open file for writing: %1").arg(file.errorString()));
        return;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    QMessageBox::information(this, tr("Export Successful"),
        tr("Whitelist configuration exported to:\n%1").arg(file_name));
}

void WhitelistTreeEditor::import_node_recursive(QTreeWidgetItem* parent, const QJsonObject& node_obj)
{
    QString name = node_obj["name"].toString();
    if (name.isEmpty()) return;
    
    QTreeWidgetItem* item;
    if (parent) {
        item = new QTreeWidgetItem(parent);
    } else {
        item = new QTreeWidgetItem(tree_widget_);
    }
    
    item->setText(0, name);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    
    // Determine depth for type label and icon
    int depth = 0;
    QTreeWidgetItem* temp = parent;
    while (temp) {
        depth++;
        temp = temp->parent();
    }
    
    if (depth == 0) {
        item->setText(1, tr("Category"));
        item->setData(0, Qt::UserRole, "category");
        item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    } else if (depth == 1) {
        item->setText(1, tr("Subcategory"));
        item->setData(0, Qt::UserRole, "subcategory");
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    } else {
        item->setText(1, tr("Level %1").arg(depth + 1));
        item->setData(0, Qt::UserRole, QString("level%1").arg(depth));
        item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
    }
    
    // Recursively import children
    if (node_obj.contains("children") && node_obj["children"].isArray()) {
        QJsonArray children = node_obj["children"].toArray();
        for (const QJsonValue& child_value : children) {
            if (child_value.isObject()) {
                import_node_recursive(item, child_value.toObject());
            }
        }
    }
}

QJsonObject WhitelistTreeEditor::export_node_recursive(QTreeWidgetItem* item) const
{
    QJsonObject node_obj;
    node_obj["name"] = item->text(0);
    
    // Export children if any
    if (item->childCount() > 0) {
        QJsonArray children;
        for (int i = 0; i < item->childCount(); ++i) {
            auto* child = item->child(i);
            if (child) {
                children.append(export_node_recursive(child));
            }
        }
        node_obj["children"] = children;
    }
    
    return node_obj;
}
