#include "WhitelistTreeEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>

WhitelistTreeEditor::WhitelistTreeEditor(const QString& name, WhitelistEntry& entry, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit Whitelist - Tree View"));
    resize(700, 600);
    setup_ui();
    
    name_edit_->setText(name);
    context_edit_->setPlainText(QString::fromStdString(entry.context));
    advanced_checkbox_->setChecked(entry.enable_advanced_subcategories);
    
    populate_tree(entry);
}

void WhitelistTreeEditor::setup_ui()
{
    auto* main_layout = new QVBoxLayout(this);
    
    // Name field
    main_layout->addWidget(new QLabel(tr("Whitelist Name:"), this));
    name_edit_ = new QLineEdit(this);
    main_layout->addWidget(name_edit_);
    
    // Tree widget with instructions
    auto* tree_label = new QLabel(tr("Categories and Subcategories (tree structure):"), this);
    tree_label->setToolTip(tr("Each category can have its own subcategories. Click + buttons to add items."));
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
    connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
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
    entry.use_hierarchical = true;
    
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
