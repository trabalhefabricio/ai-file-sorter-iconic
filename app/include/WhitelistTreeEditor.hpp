#ifndef WHITELIST_TREE_EDITOR_HPP
#define WHITELIST_TREE_EDITOR_HPP

#include <QDialog>
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QButtonGroup>
#include <QLabel>
#include "WhitelistStore.hpp"

class WhitelistTreeEditor : public QDialog {
    Q_OBJECT

public:
    explicit WhitelistTreeEditor(const QString& name, WhitelistEntry& entry, QWidget* parent = nullptr);
    
    QString get_name() const { return name_edit_->text(); }
    WhitelistEntry get_entry() const;

private slots:
    void on_add_category();
    void on_add_subcategory();
    void on_add_child_to_selected();  // Add child to any selected item
    void on_remove_item();
    void on_item_changed(QTreeWidgetItem* item, int column);
    void on_selection_changed();
    void on_mode_changed();
    void on_edit_shared_subcategories();

private:
    void setup_ui();
    void populate_tree(const WhitelistEntry& entry);
    void add_category_node(const QString& name, const QStringList& subcategories);
    void add_node_recursive(QTreeWidgetItem* parent, const CategoryNode& node);
    CategoryNode item_to_node(QTreeWidgetItem* item) const;
    QTreeWidgetItem* get_selected_category_node();
    void update_mode_ui();
    void populate_tree_for_mode();
    void enable_drag_drop();
    
    QLineEdit* name_edit_;
    QTreeWidget* tree_widget_;
    QPushButton* add_category_btn_;
    QPushButton* add_subcategory_btn_;
    QPushButton* remove_btn_;
    QTextEdit* context_edit_;
    QCheckBox* advanced_checkbox_;
    
    // Mode selection
    QButtonGroup* mode_group_;
    QRadioButton* hierarchical_mode_radio_;
    QRadioButton* shared_mode_radio_;
    
    // Shared subcategories UI
    QGroupBox* shared_subs_group_;
    QLabel* shared_subs_label_;
    QPushButton* edit_shared_subs_btn_;
    
    QStringList shared_subcategories_;  // Store shared subs when in shared mode
    
    bool updating_tree_{false};  // Prevent recursion during updates
};

#endif // WHITELIST_TREE_EDITOR_HPP
