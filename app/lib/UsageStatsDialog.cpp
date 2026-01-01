#include "UsageStatsDialog.hpp"
#include "Logger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QProgressBar>
#include <QFont>

UsageStatsDialog::UsageStatsDialog(DatabaseManager& db, QWidget* parent)
    : QDialog(parent), db_(db), tracker_(db) {
    setWindowTitle(tr("API Usage Statistics"));
    setMinimumSize(700, 500);
    
    setup_ui();
    refresh_stats();
}

void UsageStatsDialog::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    
    // Tab widget for different providers
    tab_widget_ = new QTabWidget(this);
    
    // === OpenAI Tab ===
    openai_tab_ = new QWidget();
    auto* openai_layout = new QVBoxLayout(openai_tab_);
    
    // Today's stats group
    auto* openai_today_group = new QGroupBox(tr("Today's Usage"), openai_tab_);
    auto* openai_today_layout = new QGridLayout(openai_today_group);
    
    openai_today_layout->addWidget(new QLabel(tr("Tokens Used:"), openai_today_group), 0, 0);
    openai_tokens_today_ = new QLabel("0", openai_today_group);
    QFont bold_font;
    bold_font.setBold(true);
    bold_font.setPointSize(12);
    openai_tokens_today_->setFont(bold_font);
    openai_today_layout->addWidget(openai_tokens_today_, 0, 1);
    
    openai_today_layout->addWidget(new QLabel(tr("Requests Made:"), openai_today_group), 1, 0);
    openai_requests_today_ = new QLabel("0", openai_today_group);
    openai_requests_today_->setFont(bold_font);
    openai_today_layout->addWidget(openai_requests_today_, 1, 1);
    
    openai_today_layout->addWidget(new QLabel(tr("Estimated Cost Today:"), openai_today_group), 2, 0);
    openai_cost_today_ = new QLabel("$0.00", openai_today_group);
    openai_cost_today_->setFont(bold_font);
    openai_today_layout->addWidget(openai_cost_today_, 2, 1);
    
    openai_today_layout->addWidget(new QLabel(tr("Estimated Cost (30 days):"), openai_today_group), 3, 0);
    openai_cost_month_ = new QLabel("$0.00", openai_today_group);
    openai_cost_month_->setFont(bold_font);
    openai_today_layout->addWidget(openai_cost_month_, 3, 1);
    
    openai_layout->addWidget(openai_today_group);
    
    // History table
    auto* openai_history_group = new QGroupBox(tr("Usage History (Last 30 Days)"), openai_tab_);
    auto* openai_history_layout = new QVBoxLayout(openai_history_group);
    
    openai_history_table_ = new QTableWidget(0, 4, openai_history_group);
    openai_history_table_->setHorizontalHeaderLabels({
        tr("Date"), tr("Tokens"), tr("Requests"), tr("Cost")
    });
    openai_history_table_->horizontalHeader()->setStretchLastSection(true);
    openai_history_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    openai_history_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    openai_history_layout->addWidget(openai_history_table_);
    
    openai_layout->addWidget(openai_history_group);
    
    tab_widget_->addTab(openai_tab_, tr("OpenAI"));
    
    // === Gemini Tab ===
    gemini_tab_ = new QWidget();
    auto* gemini_layout = new QVBoxLayout(gemini_tab_);
    
    // Today's stats group
    auto* gemini_today_group = new QGroupBox(tr("Today's Usage (Free Tier)"), gemini_tab_);
    auto* gemini_today_layout = new QGridLayout(gemini_today_group);
    
    gemini_today_layout->addWidget(new QLabel(tr("Requests Made:"), gemini_today_group), 0, 0);
    gemini_requests_today_ = new QLabel("0", gemini_today_group);
    gemini_requests_today_->setFont(bold_font);
    gemini_today_layout->addWidget(gemini_requests_today_, 0, 1);
    
    gemini_today_layout->addWidget(new QLabel(tr("Remaining Today:"), gemini_today_group), 1, 0);
    gemini_remaining_ = new QLabel("1500 / 1500", gemini_today_group);
    gemini_remaining_->setFont(bold_font);
    gemini_today_layout->addWidget(gemini_remaining_, 1, 1);
    
    gemini_today_layout->addWidget(new QLabel(tr("Daily Quota:"), gemini_today_group), 2, 0);
    gemini_quota_bar_ = new QProgressBar(gemini_today_group);
    gemini_quota_bar_->setRange(0, 1500);
    gemini_quota_bar_->setValue(0);
    gemini_quota_bar_->setTextVisible(true);
    gemini_quota_bar_->setFormat("%v / %m");
    gemini_today_layout->addWidget(gemini_quota_bar_, 2, 1);
    
    // Note about limits
    auto* note_label = new QLabel(
        tr("<i>Free tier: 15 requests/minute, 1500 requests/day</i>"),
        gemini_today_group
    );
    note_label->setWordWrap(true);
    gemini_today_layout->addWidget(note_label, 3, 0, 1, 2);
    
    gemini_layout->addWidget(gemini_today_group);
    
    // History table
    auto* gemini_history_group = new QGroupBox(tr("Usage History (Last 30 Days)"), gemini_tab_);
    auto* gemini_history_layout = new QVBoxLayout(gemini_history_group);
    
    gemini_history_table_ = new QTableWidget(0, 3, gemini_history_group);
    gemini_history_table_->setHorizontalHeaderLabels({
        tr("Date"), tr("Requests"), tr("Remaining")
    });
    gemini_history_table_->horizontalHeader()->setStretchLastSection(true);
    gemini_history_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    gemini_history_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    gemini_history_layout->addWidget(gemini_history_table_);
    
    gemini_layout->addWidget(gemini_history_group);
    
    tab_widget_->addTab(gemini_tab_, tr("Gemini"));
    
    main_layout->addWidget(tab_widget_);
    
    // Buttons
    auto* button_layout = new QHBoxLayout();
    refresh_button_ = new QPushButton(tr("Refresh"), this);
    close_button_ = new QPushButton(tr("Close"), this);
    
    button_layout->addStretch();
    button_layout->addWidget(refresh_button_);
    button_layout->addWidget(close_button_);
    
    main_layout->addLayout(button_layout);
    
    // Connect signals
    connect(refresh_button_, &QPushButton::clicked, this, &UsageStatsDialog::refresh_stats);
    connect(close_button_, &QPushButton::clicked, this, &QDialog::accept);
    connect(tab_widget_, &QTabWidget::currentChanged, this, &UsageStatsDialog::on_tab_changed);
}

void UsageStatsDialog::refresh_stats() {
    update_openai_stats();
    update_gemini_stats();
}

void UsageStatsDialog::on_tab_changed(int index) {
    // Refresh the active tab
    if (index == 0) {
        update_openai_stats();
    } else if (index == 1) {
        update_gemini_stats();
    }
}

void UsageStatsDialog::update_openai_stats() {
    auto stats = tracker_.get_stats("openai");
    
    openai_tokens_today_->setText(format_tokens(stats.tokens_used_today));
    openai_requests_today_->setText(QString::number(stats.requests_today));
    openai_cost_today_->setText(format_cost(stats.estimated_cost_today));
    openai_cost_month_->setText(format_cost(stats.estimated_cost_month));
    
    populate_history_table(openai_history_table_, "openai");
}

void UsageStatsDialog::update_gemini_stats() {
    auto stats = tracker_.get_stats("gemini");
    
    gemini_requests_today_->setText(QString::number(stats.requests_today));
    gemini_remaining_->setText(QString("%1 / %2")
        .arg(stats.remaining_free_requests)
        .arg(APIUsageTracker::GEMINI_FREE_RPD));
    
    // Update progress bar
    if (gemini_quota_bar_) {
        gemini_quota_bar_->setValue(stats.requests_today);
        
        // Change color based on usage
        QString style;
        if (stats.requests_today >= APIUsageTracker::GEMINI_FREE_RPD * 0.9) {
            style = "QProgressBar::chunk { background-color: #d32f2f; }";  // Red
        } else if (stats.requests_today >= APIUsageTracker::GEMINI_FREE_RPD * 0.7) {
            style = "QProgressBar::chunk { background-color: #ffa726; }";  // Orange
        } else {
            style = "QProgressBar::chunk { background-color: #66bb6a; }";  // Green
        }
        gemini_quota_bar_->setStyleSheet(style);
    }
    
    populate_history_table(gemini_history_table_, "gemini");
}

void UsageStatsDialog::populate_history_table(QTableWidget* table, const std::string& provider) {
    auto history = db_.get_api_usage_history(provider, 30);
    
    table->setRowCount(0);
    table->setRowCount(history.size());
    
    for (size_t i = 0; i < history.size(); ++i) {
        const auto& entry = history[i];
        
        table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(entry.date)));
        
        if (provider == "openai") {
            table->setItem(i, 1, new QTableWidgetItem(format_tokens(entry.tokens_used)));
            table->setItem(i, 2, new QTableWidgetItem(QString::number(entry.requests_made)));
            table->setItem(i, 3, new QTableWidgetItem(format_cost(entry.cost_estimate)));
        } else {  // gemini
            table->setItem(i, 1, new QTableWidgetItem(QString::number(entry.requests_made)));
            table->setItem(i, 2, new QTableWidgetItem(QString::number(entry.remaining)));
        }
    }
    
    // Resize columns to content
    table->resizeColumnsToContents();
}

QString UsageStatsDialog::format_cost(float cost) const {
    if (cost < 0.01f && cost > 0.0f) {
        return QString("< $0.01");
    }
    return QString("$%1").arg(cost, 0, 'f', 2);
}

QString UsageStatsDialog::format_tokens(int tokens) const {
    if (tokens >= 1000000) {
        return QString("%1M").arg(tokens / 1000000.0, 0, 'f', 2);
    } else if (tokens >= 1000) {
        return QString("%1K").arg(tokens / 1000.0, 0, 'f', 1);
    }
    return QString::number(tokens);
}
