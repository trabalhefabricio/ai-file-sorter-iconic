#ifndef CATEGORY_SUGGESTION_WIZARD_HPP
#define CATEGORY_SUGGESTION_WIZARD_HPP

#include <QDialog>
#include <QString>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <string>
#include <vector>

#include "Types.hpp"

class CategorySuggestionWizard : public QDialog {
    Q_OBJECT

public:
    enum WizardResult {
        UseParent,    // Place file at parent level
        CreateNew,    // Create new subcategory
        Skip          // Skip this file
    };

    CategorySuggestionWizard(const FileEntry& file,
                            const std::string& suggested_parent,
                            double confidence_score,
                            const std::vector<std::string>& existing_paths,
                            QWidget* parent = nullptr);

    WizardResult get_result() const { return result_; }
    std::string get_path() const { return selected_path_; }
    std::string get_parent_path() const { return parent_path_; }
    bool apply_to_similar() const { return apply_to_similar_checkbox_->isChecked(); }

private slots:
    void on_radio_changed();
    void on_create_clicked();
    void on_cancel_clicked();
    void on_subcategory_input_changed(const QString& text);

private:
    void setup_ui();
    void create_file_preview_section();
    void create_options_section();
    void create_buttons_section();
    void load_file_preview();
    QString validate_input();
    QString format_file_size(qint64 size);
    bool is_valid_path_segment(const std::string& segment);
    int count_path_depth(const std::string& path);

    // File and suggestion data
    FileEntry file_;
    std::string parent_path_;
    double confidence_score_;
    std::vector<std::string> existing_paths_;

    // Result data
    WizardResult result_;
    std::string selected_path_;

    // UI Components
    QLabel* file_preview_label_;
    QLabel* file_info_label_;
    QLabel* ai_suggestion_label_;
    
    QButtonGroup* option_group_;
    QRadioButton* use_parent_radio_;
    QRadioButton* create_new_radio_;
    QRadioButton* skip_radio_;
    
    QLabel* subcategory_label_;
    QLineEdit* subcategory_input_;
    QLabel* validation_label_;
    
    QCheckBox* apply_to_similar_checkbox_;
    
    QPushButton* create_button_;
    QPushButton* cancel_button_;
};

#endif // CATEGORY_SUGGESTION_WIZARD_HPP
