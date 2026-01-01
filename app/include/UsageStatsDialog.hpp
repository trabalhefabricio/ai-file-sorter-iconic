#ifndef USAGESTATSDIALOG_HPP
#define USAGESTATSDIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include "APIUsageTracker.hpp"
#include "DatabaseManager.hpp"

/**
 * @brief Dialog for displaying API usage statistics
 * 
 * Shows current usage for OpenAI and Gemini APIs, including:
 * - Today's token/request usage
 * - Estimated costs
 * - Remaining free tier quota (Gemini)
 * - Historical usage
 */
class UsageStatsDialog : public QDialog {
    Q_OBJECT

public:
    explicit UsageStatsDialog(DatabaseManager& db, QWidget* parent = nullptr);

private slots:
    void refresh_stats();
    void on_tab_changed(int index);

private:
    void setup_ui();
    void update_openai_stats();
    void update_gemini_stats();
    void populate_history_table(QTableWidget* table, const std::string& provider);
    
    QString format_cost(float cost);
    QString format_tokens(int tokens);
    
    DatabaseManager& db_;
    APIUsageTracker tracker_;
    
    // UI components
    QTabWidget* tab_widget_;
    
    // OpenAI tab
    QWidget* openai_tab_;
    QLabel* openai_tokens_today_;
    QLabel* openai_requests_today_;
    QLabel* openai_cost_today_;
    QLabel* openai_cost_month_;
    QTableWidget* openai_history_table_;
    
    // Gemini tab
    QWidget* gemini_tab_;
    QLabel* gemini_requests_today_;
    QLabel* gemini_remaining_;
    QLabel* gemini_quota_bar_;
    QTableWidget* gemini_history_table_;
    
    QPushButton* refresh_button_;
    QPushButton* close_button_;
};

#endif // USAGESTATSDIALOG_HPP
