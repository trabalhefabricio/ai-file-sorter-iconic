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
    tree_widget_->setEditTriggers(QAbstractItemView::DoubleClickEdit);
    tree_widget_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_widget_->setAlternatingRowColors(true);
    main_layout->addWidget(tree_widget_);
    
    // Buttons row
    auto* btn_layout = new QHBoxLayout();
    
    add_category_btn_ = new QPushButton(tr("+ Category"), this);
    add_category_btn_->setToolTip(tr("Add a new category at the root level"));
    add_category_btn_->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    btn_layout->addWidget(add_category_btn_);
    
    add_subcategory_btn_ = new QPushButton(tr("+ Subcategory"), this);
    add_subcategory_btn_->setToolTip(tr("Add a subcategory to the selected category"));
    add_subcategory_btn_->setIcon(style()->standardIcon(QStyle::SP_FileLinkIcon));
    add_subcategory_btn_->setEnabled(false);
    btn_layout->addWidget(add_subcategory_btn_);
    
    remove_btn_ = new QPushButton(tr("Remove"), this);
    remove_btn_->setToolTip(tr("Remove selected item"));
    remove_btn_->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    remove_btn_->setEnabled(false);
    btn_layout->addWidget(remove_btn_);
    
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
    connect(add_subcategory_btn_, &QPushButton::clicked, this, &WhitelistTreeEditor::on_add_subcategory);
    connect(remove_btn_, &QPushButton::clicked, this, &WhitelistTreeEditor::on_remove_item);
    connect(tree_widget_, &QTreeWidget::itemChanged, this, &WhitelistTreeEditor::on_item_changed);
    connect(tree_widget_, &QTreeWidget::itemSelectionChanged, this, &WhitelistTreeEditor::on_selection_changed);
    connect(mode_group_, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), 
            this, &WhitelistTreeEditor::on_mode_changed);
    connect(edit_shared_subs_btn_, &QPushButton::clicked, this, &WhitelistTreeEditor::on_edit_shared_subcategories);
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

void WhitelistTreeEditor::populate_tree_for_mode()
{
    tree_widget_->clear();
    updating_tree_ = true;
    
    // Get all categories from tree
    QStringList categories;
    for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
        categories << tree_widget_->topLevelItem(i)->text(0);
    }
    
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
            // Collect all unique subcategories
            QSet<QString> unique_subs;
            for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
                auto* cat_item = tree_widget_->topLevelItem(i);
                for (int j = 0; j < cat_item->childCount(); ++j) {
                    unique_subs.insert(cat_item->child(j)->text(0));
                }
            }
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
        QStringList subs;
        for (const auto& sub : node.subcategories) {
            subs << QString::fromStdString(sub);
        }
        add_category_node(QString::fromStdString(node.name), subs);
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
        // Hierarchical mode: each category has its own subcategories
        std::vector<CategoryNode> nodes;
        
        for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
            auto* cat_item = tree_widget_->topLevelItem(i);
            CategoryNode node;
            node.name = cat_item->text(0).toStdString();
            
            for (int j = 0; j < cat_item->childCount(); ++j) {
                auto* sub_item = cat_item->child(j);
                node.subcategories.push_back(sub_item->text(0).toStdString());
            }
            
            nodes.push_back(node);
        }
        
        entry.from_tree(nodes);
        entry.flatten_to_legacy();  // Also populate flat structure for compatibility
    } else {
        // Shared mode: all categories use the same subcategories (classic mode)
        entry.categories.clear();
        entry.subcategories.clear();
        
        for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
            auto* cat_item = tree_widget_->topLevelItem(i);
            entry.categories.push_back(cat_item->text(0).toStdString());
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
    auto* category_node = get_selected_category_node();
    if (!category_node) {
        QMessageBox::warning(this, tr("No Category Selected"),
                           tr("Please select a category first."));
        return;
    }
    
    bool ok;
    QString name = QInputDialog::getText(this,
                                         tr("Add Subcategory"),
                                         tr("Subcategory name:"),
                                         QLineEdit::Normal,
                                         QString(),
                                         &ok);
    if (ok && !name.isEmpty()) {
        updating_tree_ = true;
        auto* sub_item = new QTreeWidgetItem(category_node);
        sub_item->setText(0, name);
        sub_item->setText(1, tr("Subcategory"));
        sub_item->setFlags(sub_item->flags() | Qt::ItemIsEditable);
        sub_item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        sub_item->setData(0, Qt::UserRole, "subcategory");
        category_node->setExpanded(true);
        updating_tree_ = false;
    }
}

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
            item->parent()->removeChild(item);
        } else {
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
    
    QString type = item->data(0, Qt::UserRole).toString();
    add_subcategory_btn_->setEnabled(type == "category");
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
