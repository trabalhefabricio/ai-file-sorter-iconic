#include "ErrorResolutionDialog.hpp"
#include "Logger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTabWidget>
#include <QProgressBar>
#include <QLineEdit>
#include <QHeaderView>
#include <QFont>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <sstream>

ErrorResolutionDialog::ErrorResolutionDialog(ErrorCodes::Code error_code,
                                            const std::string& context,
                                            std::shared_ptr<AIErrorResolver> resolver,
                                            QWidget* parent)
    : QDialog(parent)
    , error_code_(error_code)
    , context_(context)
    , user_description_("")
    , resolver_(resolver)
    , analysis_complete_(false)
    , tab_widget_(nullptr)
    , error_code_label_(nullptr)
    , user_input_text_(nullptr)
    , analyze_button_(nullptr)
    , input_help_label_(nullptr)
    , category_label_(nullptr)
    , confidence_label_(nullptr)
    , diagnosis_text_(nullptr)
    , explanation_text_(nullptr)
    , analysis_progress_(nullptr)
    , steps_tree_(nullptr)
    , try_fix_button_(nullptr)
    , fix_status_label_(nullptr)
    , fix_progress_(nullptr)
    , history_tree_(nullptr)
    , refresh_history_button_(nullptr)
    , copy_button_(nullptr)
    , close_button_(nullptr)
{
    setWindowTitle("AI Error Resolution Assistant");
    setMinimumSize(900, 700);
    setup_ui();
    
    // Automatically perform analysis on construction
    perform_analysis();
}

ErrorResolutionDialog::ErrorResolutionDialog(const std::string& user_description,
                                            std::shared_ptr<AIErrorResolver> resolver,
                                            QWidget* parent)
    : QDialog(parent)
    , error_code_(ErrorCodes::Code::UNKNOWN_ERROR)
    , context_("")
    , user_description_(user_description)
    , resolver_(resolver)
    , analysis_complete_(false)
    , tab_widget_(nullptr)
    , error_code_label_(nullptr)
    , user_input_text_(nullptr)
    , analyze_button_(nullptr)
    , input_help_label_(nullptr)
    , category_label_(nullptr)
    , confidence_label_(nullptr)
    , diagnosis_text_(nullptr)
    , explanation_text_(nullptr)
    , analysis_progress_(nullptr)
    , steps_tree_(nullptr)
    , try_fix_button_(nullptr)
    , fix_status_label_(nullptr)
    , fix_progress_(nullptr)
    , history_tree_(nullptr)
    , refresh_history_button_(nullptr)
    , copy_button_(nullptr)
    , close_button_(nullptr)
{
    setWindowTitle("AI Error Resolution Assistant");
    setMinimumSize(900, 700);
    setup_ui();
    
    // Parse natural language description to identify error
    auto [category, potential_codes] = resolver_->parse_natural_language_error(user_description);
    if (!potential_codes.empty()) {
        error_code_ = potential_codes[0];
    }
    
    // Set user input text
    user_input_text_->setText(QString::fromStdString(user_description));
}

void ErrorResolutionDialog::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    
    // Create tab widget
    tab_widget_ = new QTabWidget(this);
    
    setup_input_tab();
    setup_analysis_tab();
    setup_resolution_tab();
    setup_history_tab();
    
    main_layout->addWidget(tab_widget_);
    
    // Bottom buttons
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    
    copy_button_ = new QPushButton("Copy Details", this);
    connect(copy_button_, &QPushButton::clicked, this, &ErrorResolutionDialog::on_copy_details_clicked);
    button_layout->addWidget(copy_button_);
    
    close_button_ = new QPushButton("Close", this);
    connect(close_button_, &QPushButton::clicked, this, &QDialog::accept);
    button_layout->addWidget(close_button_);
    
    main_layout->addLayout(button_layout);
}

void ErrorResolutionDialog::setup_input_tab() {
    auto* input_widget = new QWidget();
    auto* layout = new QVBoxLayout(input_widget);
    
    // Error code display
    error_code_label_ = new QLabel();
    error_code_label_->setText(QString("Error Code: %1").arg(static_cast<int>(error_code_)));
    QFont bold_font = error_code_label_->font();
    bold_font.setBold(true);
    bold_font.setPointSize(bold_font.pointSize() + 2);
    error_code_label_->setFont(bold_font);
    layout->addWidget(error_code_label_);
    
    // Help label
    input_help_label_ = new QLabel(
        "Describe what happened in your own words. The AI will analyze your description "
        "along with the error details to provide personalized help."
    );
    input_help_label_->setWordWrap(true);
    layout->addWidget(input_help_label_);
    
    layout->addSpacing(10);
    
    // User input area
    auto* input_label = new QLabel("What problem are you experiencing?");
    input_label->setFont(bold_font);
    layout->addWidget(input_label);
    
    user_input_text_ = new QTextEdit();
    user_input_text_->setPlaceholderText(
        "Example: The app won't connect to Gemini and keeps timing out after a few seconds..."
    );
    user_input_text_->setMaximumHeight(150);
    layout->addWidget(user_input_text_);
    
    // Analyze button
    analyze_button_ = new QPushButton("Analyze Problem", this);
    analyze_button_->setStyleSheet("QPushButton { padding: 8px 16px; font-weight: bold; }");
    connect(analyze_button_, &QPushButton::clicked, this, &ErrorResolutionDialog::on_analyze_clicked);
    layout->addWidget(analyze_button_);
    
    layout->addStretch();
    
    tab_widget_->addTab(input_widget, "Input");
}

void ErrorResolutionDialog::setup_analysis_tab() {
    auto* analysis_widget = new QWidget();
    auto* layout = new QVBoxLayout(analysis_widget);
    
    // Category and confidence
    auto* info_layout = new QHBoxLayout();
    
    category_label_ = new QLabel("Category: Unknown");
    QFont bold_font = category_label_->font();
    bold_font.setBold(true);
    category_label_->setFont(bold_font);
    info_layout->addWidget(category_label_);
    
    info_layout->addStretch();
    
    confidence_label_ = new QLabel("Confidence: 0%");
    confidence_label_->setFont(bold_font);
    info_layout->addWidget(confidence_label_);
    
    layout->addLayout(info_layout);
    
    // Explanation
    auto* exp_label = new QLabel("What Happened:");
    exp_label->setFont(bold_font);
    layout->addWidget(exp_label);
    
    explanation_text_ = new QTextEdit();
    explanation_text_->setReadOnly(true);
    explanation_text_->setMaximumHeight(120);
    layout->addWidget(explanation_text_);
    
    // Diagnosis
    auto* diag_label = new QLabel("AI Diagnosis:");
    diag_label->setFont(bold_font);
    layout->addWidget(diag_label);
    
    diagnosis_text_ = new QTextEdit();
    diagnosis_text_->setReadOnly(true);
    layout->addWidget(diagnosis_text_);
    
    // Progress bar
    analysis_progress_ = new QProgressBar();
    analysis_progress_->setVisible(false);
    layout->addWidget(analysis_progress_);
    
    tab_widget_->addTab(analysis_widget, "Analysis");
}

void ErrorResolutionDialog::setup_resolution_tab() {
    auto* resolution_widget = new QWidget();
    auto* layout = new QVBoxLayout(resolution_widget);
    
    QFont bold_font;
    bold_font.setBold(true);
    
    auto* steps_label = new QLabel("Resolution Steps:");
    steps_label->setFont(bold_font);
    layout->addWidget(steps_label);
    
    auto* help_label = new QLabel(
        "Follow these steps to resolve the error. Steps marked with ⚙ can be attempted automatically."
    );
    help_label->setWordWrap(true);
    layout->addWidget(help_label);
    
    // Steps tree
    steps_tree_ = new QTreeWidget();
    steps_tree_->setHeaderLabels({"Step", "Auto-Fix Available"});
    steps_tree_->setColumnWidth(0, 600);
    steps_tree_->setAlternatingRowColors(true);
    connect(steps_tree_, &QTreeWidget::itemClicked, 
            this, &ErrorResolutionDialog::on_resolution_step_clicked);
    layout->addWidget(steps_tree_);
    
    // Try fix button
    auto* fix_layout = new QHBoxLayout();
    
    try_fix_button_ = new QPushButton("⚙ Try Automated Fixes", this);
    try_fix_button_->setStyleSheet(
        "QPushButton { padding: 8px 16px; font-weight: bold; background-color: #4CAF50; color: white; }"
        "QPushButton:disabled { background-color: #cccccc; }"
    );
    try_fix_button_->setEnabled(false);
    connect(try_fix_button_, &QPushButton::clicked, this, &ErrorResolutionDialog::on_try_fix_clicked);
    fix_layout->addWidget(try_fix_button_);
    
    fix_layout->addStretch();
    
    fix_status_label_ = new QLabel();
    fix_layout->addWidget(fix_status_label_);
    
    layout->addLayout(fix_layout);
    
    // Fix progress
    fix_progress_ = new QProgressBar();
    fix_progress_->setVisible(false);
    layout->addWidget(fix_progress_);
    
    tab_widget_->addTab(resolution_widget, "Resolution");
}

void ErrorResolutionDialog::setup_history_tab() {
    auto* history_widget = new QWidget();
    auto* layout = new QVBoxLayout(history_widget);
    
    QFont bold_font;
    bold_font.setBold(true);
    
    auto* history_label = new QLabel("Resolution History:");
    history_label->setFont(bold_font);
    layout->addWidget(history_label);
    
    auto* help_label = new QLabel(
        "Past resolution attempts for similar errors. This helps the AI learn from previous fixes."
    );
    help_label->setWordWrap(true);
    layout->addWidget(help_label);
    
    // History tree
    history_tree_ = new QTreeWidget();
    history_tree_->setHeaderLabels({"Date", "Error Code", "Result", "Steps Taken"});
    history_tree_->setAlternatingRowColors(true);
    layout->addWidget(history_tree_);
    
    // Refresh button
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    
    refresh_history_button_ = new QPushButton("Refresh History", this);
    connect(refresh_history_button_, &QPushButton::clicked, 
            this, &ErrorResolutionDialog::on_view_history_clicked);
    button_layout->addWidget(refresh_history_button_);
    
    layout->addLayout(button_layout);
    
    tab_widget_->addTab(history_widget, "History");
}

void ErrorResolutionDialog::on_analyze_clicked() {
    user_description_ = user_input_text_->toPlainText().toStdString();
    perform_analysis();
}

void ErrorResolutionDialog::perform_analysis() {
    if (!resolver_) {
        QMessageBox::warning(this, "Error", "AI Error Resolver not available.");
        return;
    }
    
    // Show progress
    update_progress("Analyzing error...", 30);
    
    try {
        // Perform AI analysis
        current_analysis_ = resolver_->analyze_error(error_code_, context_, user_description_);
        analysis_complete_ = true;
        
        update_progress("Analysis complete", 100);
        
        // Display results
        display_analysis_results();
        populate_resolution_steps();
        populate_history();
        
        // Switch to analysis tab
        tab_widget_->setCurrentIndex(1);
        
    } catch (const std::exception& e) {
        Logger::log_error("ErrorResolutionDialog: Analysis failed: " + std::string(e.what()));
        QMessageBox::critical(this, "Analysis Error", 
            QString("Failed to analyze error: %1").arg(e.what()));
        update_progress("Analysis failed", 0);
    }
}

void ErrorResolutionDialog::display_analysis_results() {
    // Update category
    std::string category_str;
    switch (current_analysis_.category) {
        case AIErrorResolver::ErrorCategory::Network: category_str = "Network"; break;
        case AIErrorResolver::ErrorCategory::API: category_str = "API"; break;
        case AIErrorResolver::ErrorCategory::FileSystem: category_str = "File System"; break;
        case AIErrorResolver::ErrorCategory::Database: category_str = "Database"; break;
        case AIErrorResolver::ErrorCategory::LLM: category_str = "LLM/AI Model"; break;
        case AIErrorResolver::ErrorCategory::Configuration: category_str = "Configuration"; break;
        case AIErrorResolver::ErrorCategory::Validation: category_str = "Validation"; break;
        case AIErrorResolver::ErrorCategory::System: category_str = "System"; break;
        case AIErrorResolver::ErrorCategory::Categorization: category_str = "Categorization"; break;
        case AIErrorResolver::ErrorCategory::Download: category_str = "Download"; break;
        default: category_str = "Unknown"; break;
    }
    category_label_->setText(QString("Category: %1").arg(QString::fromStdString(category_str)));
    
    // Update confidence
    int confidence_pct = static_cast<int>(current_analysis_.confidence_score * 100);
    confidence_label_->setText(QString("Confidence: %1%").arg(confidence_pct));
    
    // Set color based on confidence
    if (confidence_pct >= 70) {
        confidence_label_->setStyleSheet("QLabel { color: green; }");
    } else if (confidence_pct >= 40) {
        confidence_label_->setStyleSheet("QLabel { color: orange; }");
    } else {
        confidence_label_->setStyleSheet("QLabel { color: red; }");
    }
    
    // Update explanation
    if (!current_analysis_.user_friendly_explanation.empty()) {
        explanation_text_->setText(QString::fromStdString(current_analysis_.user_friendly_explanation));
    } else {
        explanation_text_->setText("No explanation available.");
    }
    
    // Update diagnosis
    diagnosis_text_->setText(QString::fromStdString(current_analysis_.ai_diagnosis));
}

void ErrorResolutionDialog::populate_resolution_steps() {
    steps_tree_->clear();
    
    bool has_auto_fix = false;
    
    int step_num = 1;
    for (const auto& step : current_analysis_.resolution_steps) {
        auto* item = new QTreeWidgetItem(steps_tree_);
        
        QString step_text = QString("%1. %2")
            .arg(step_num++)
            .arg(QString::fromStdString(step.description));
        
        item->setText(0, step_text);
        item->setText(1, step.can_auto_fix ? "⚙ Yes" : "No");
        
        if (step.can_auto_fix) {
            has_auto_fix = true;
            item->setForeground(1, QBrush(QColor(76, 175, 80)));  // Green
        }
        
        if (!step.technical_detail.empty()) {
            item->setToolTip(0, QString::fromStdString(step.technical_detail));
        }
    }
    
    // Enable try fix button if any auto-fixable steps exist
    try_fix_button_->setEnabled(has_auto_fix);
    
    if (has_auto_fix) {
        fix_status_label_->setText("Automated fixes available");
        fix_status_label_->setStyleSheet("QLabel { color: green; }");
    } else {
        fix_status_label_->setText("No automated fixes available");
        fix_status_label_->setStyleSheet("QLabel { color: gray; }");
    }
}

void ErrorResolutionDialog::populate_history() {
    history_tree_->clear();
    
    try {
        auto history = resolver_->get_resolution_history(error_code_, 20);
        
        for (const auto& entry : history) {
            auto* item = new QTreeWidgetItem(history_tree_);
            
            // TODO: Add timestamp when database integration is complete
            item->setText(0, "Recent");
            item->setText(1, QString::number(static_cast<int>(error_code_)));
            item->setText(2, entry.success ? "Success" : "Failed");
            
            QString steps_summary;
            for (size_t i = 0; i < entry.steps_taken.size() && i < 3; ++i) {
                if (i > 0) steps_summary += "; ";
                steps_summary += QString::fromStdString(entry.steps_taken[i]);
            }
            if (entry.steps_taken.size() > 3) {
                steps_summary += "...";
            }
            item->setText(3, steps_summary);
            
            // Color code by result
            if (entry.success) {
                item->setForeground(2, QBrush(QColor(76, 175, 80)));  // Green
            } else {
                item->setForeground(2, QBrush(QColor(244, 67, 54)));  // Red
            }
        }
    } catch (const std::exception& e) {
        Logger::log_error("ErrorResolutionDialog: Failed to load history: " + std::string(e.what()));
    }
}

void ErrorResolutionDialog::on_try_fix_clicked() {
    if (!analysis_complete_) {
        QMessageBox::warning(this, "Not Ready", "Please analyze the error first.");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Automated Fix",
        "This will attempt to automatically fix the error. Do you want to proceed?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        attempt_automated_fix();
    }
}

void ErrorResolutionDialog::attempt_automated_fix() {
    update_progress("Attempting automated fixes...", 10);
    fix_progress_->setVisible(true);
    try_fix_button_->setEnabled(false);
    
    try {
        auto result = resolver_->attempt_auto_resolution(current_analysis_);
        
        update_progress("Fix attempt complete", 100);
        show_result_message(result.success, QString::fromStdString(result.message));
        
        // Refresh history
        populate_history();
        
    } catch (const std::exception& e) {
        Logger::log_error("ErrorResolutionDialog: Auto-fix failed: " + std::string(e.what()));
        show_result_message(false, QString("Exception: %1").arg(e.what()));
    }
    
    fix_progress_->setVisible(false);
    try_fix_button_->setEnabled(true);
}

void ErrorResolutionDialog::on_copy_details_clicked() {
    std::stringstream details;
    details << "Error Resolution Details\n";
    details << "========================\n\n";
    details << "Error Code: " << static_cast<int>(error_code_) << "\n";
    details << "Context: " << context_ << "\n\n";
    
    if (analysis_complete_) {
        details << "Analysis:\n";
        details << current_analysis_.ai_diagnosis << "\n\n";
        details << "Resolution Steps:\n";
        int step_num = 1;
        for (const auto& step : current_analysis_.resolution_steps) {
            details << step_num++ << ". " << step.description << "\n";
        }
    }
    
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(QString::fromStdString(details.str()));
    
    QMessageBox::information(this, "Copied", "Error details copied to clipboard.");
}

void ErrorResolutionDialog::on_view_history_clicked() {
    populate_history();
}

void ErrorResolutionDialog::on_resolution_step_clicked(QTreeWidgetItem* item, int column) {
    // Show tooltip or additional info
    if (item) {
        QString tooltip = item->toolTip(0);
        if (!tooltip.isEmpty()) {
            QMessageBox::information(this, "Step Details", tooltip);
        }
    }
}

void ErrorResolutionDialog::update_progress(const QString& message, int percent) {
    if (analysis_progress_) {
        analysis_progress_->setVisible(true);
        analysis_progress_->setValue(percent);
        analysis_progress_->setFormat(message + " - %p%");
    }
    
    if (fix_progress_) {
        fix_progress_->setValue(percent);
        fix_progress_->setFormat(message + " - %p%");
    }
}

void ErrorResolutionDialog::show_result_message(bool success, const QString& message) {
    if (success) {
        fix_status_label_->setText("✓ " + message);
        fix_status_label_->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        QMessageBox::information(this, "Success", message);
    } else {
        fix_status_label_->setText("✗ " + message);
        fix_status_label_->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        QMessageBox::warning(this, "Fix Failed", message);
    }
}
