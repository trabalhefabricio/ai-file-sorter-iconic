#ifndef WHITELIST_TREE_EDITOR_HPP
#define WHITELIST_TREE_EDITOR_HPP

#include <QDialog>
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
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
    void on_remove_item();
    void on_item_changed(QTreeWidgetItem* item, int column);
    void on_selection_changed();

private:
    void setup_ui();
    void populate_tree(const WhitelistEntry& entry);
    void add_category_node(const QString& name, const QStringList& subcategories);
    QTreeWidgetItem* get_selected_category_node();
    
    QLineEdit* name_edit_;
    QTreeWidget* tree_widget_;
    QPushButton* add_category_btn_;
    QPushButton* add_subcategory_btn_;
    QPushButton* remove_btn_;
    QTextEdit* context_edit_;
    QCheckBox* advanced_checkbox_;
    
    bool updating_tree_{false};  // Prevent recursion during updates
};

#endif // WHITELIST_TREE_EDITOR_HPP
