#ifndef ERROR_RESOLUTION_DIALOG_HPP
#define ERROR_RESOLUTION_DIALOG_HPP

#include "AIErrorResolver.hpp"
#include "ErrorCode.hpp"
#include <QDialog>
#include <memory>

class QTextEdit;
class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QTabWidget;
class QProgressBar;
class QLineEdit;

/**
 * @brief Dialog for AI-assisted error resolution
 * 
 * Provides an interactive interface for users to:
 * - Describe errors in natural language
 * - View AI diagnosis and analysis
 * - See suggested resolution steps
 * - Attempt automated fixes with user approval
 */
class ErrorResolutionDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct error resolution dialog
     * @param error_code The error code that occurred
     * @param context Additional error context
     * @param resolver AI error resolver instance
     * @param parent Parent widget
     */
    explicit ErrorResolutionDialog(ErrorCodes::Code error_code,
                                   const std::string& context,
                                   std::shared_ptr<AIErrorResolver> resolver,
                                   QWidget* parent = nullptr);

    /**
     * @brief Construct error resolution dialog with user description
     * @param user_description Natural language error description from user
     * @param resolver AI error resolver instance
     * @param parent Parent widget
     */
    explicit ErrorResolutionDialog(const std::string& user_description,
                                   std::shared_ptr<AIErrorResolver> resolver,
                                   QWidget* parent = nullptr);

    ~ErrorResolutionDialog() override = default;

private slots:
    void on_analyze_clicked();
    void on_try_fix_clicked();
    void on_copy_details_clicked();
    void on_view_history_clicked();
    void on_resolution_step_clicked(QTreeWidgetItem* item, int column);

private:
    void setup_ui();
    void setup_input_tab();
    void setup_analysis_tab();
    void setup_resolution_tab();
    void setup_history_tab();
    
    void perform_analysis();
    void display_analysis_results();
    void populate_resolution_steps();
    void populate_history();
    void attempt_automated_fix();
    void update_progress(const QString& message, int percent);
    void show_result_message(bool success, const QString& message);
    
    ErrorCodes::Code error_code_;
    std::string context_;
    std::string user_description_;
    std::shared_ptr<AIErrorResolver> resolver_;
    
    AIErrorResolver::ErrorAnalysis current_analysis_;
    bool analysis_complete_;
    
    // UI elements
    QTabWidget* tab_widget_;
    
    // Input tab
    QLabel* error_code_label_;
    QTextEdit* user_input_text_;
    QPushButton* analyze_button_;
    QLabel* input_help_label_;
    
    // Analysis tab
    QLabel* category_label_;
    QLabel* confidence_label_;
    QTextEdit* diagnosis_text_;
    QTextEdit* explanation_text_;
    QProgressBar* analysis_progress_;
    
    // Resolution tab
    QTreeWidget* steps_tree_;
    QPushButton* try_fix_button_;
    QLabel* fix_status_label_;
    QProgressBar* fix_progress_;
    
    // History tab
    QTreeWidget* history_tree_;
    QPushButton* refresh_history_button_;
    
    // Bottom buttons
    QPushButton* copy_button_;
    QPushButton* close_button_;
};

#endif // ERROR_RESOLUTION_DIALOG_HPP
