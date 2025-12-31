#include "CacheManagerDialog.hpp"
#include "DatabaseManager.hpp"
#include "DialogUtils.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

CacheManagerDialog::CacheManagerDialog(DatabaseManager& db, QWidget* parent)
    : QDialog(parent), db_(db) {
    setWindowTitle("Cache Management");
    setMinimumWidth(500);
    setMinimumHeight(400);
    
    setup_ui();
    update_statistics();
}

void CacheManagerDialog::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    
    // Statistics group
    auto* stats_group = new QGroupBox("Cache Statistics", this);
    auto* stats_layout = new QGridLayout(stats_group);
    
    int row = 0;
    
    stats_layout->addWidget(new QLabel("Cache Entries:", this), row, 0);
    entry_count_label_ = new QLabel("0", this);
    entry_count_label_->setStyleSheet("font-weight: bold;");
    stats_layout->addWidget(entry_count_label_, row++, 1);
    
    stats_layout->addWidget(new QLabel("Database Size:", this), row, 0);
    database_size_label_ = new QLabel("0 bytes", this);
    database_size_label_->setStyleSheet("font-weight: bold;");
    stats_layout->addWidget(database_size_label_, row++, 1);
    
    stats_layout->addWidget(new QLabel("Distinct Folders:", this), row, 0);
    folders_count_label_ = new QLabel("0", this);
    folders_count_label_->setStyleSheet("font-weight: bold;");
    stats_layout->addWidget(folders_count_label_, row++, 1);
    
    stats_layout->addWidget(new QLabel("Oldest Entry:", this), row, 0);
    oldest_entry_label_ = new QLabel("N/A", this);
    oldest_entry_label_->setStyleSheet("font-weight: bold;");
    stats_layout->addWidget(oldest_entry_label_, row++, 1);
    
    stats_layout->addWidget(new QLabel("Newest Entry:", this), row, 0);
    newest_entry_label_ = new QLabel("N/A", this);
    newest_entry_label_->setStyleSheet("font-weight: bold;");
    stats_layout->addWidget(newest_entry_label_, row++, 1);
    
    main_layout->addWidget(stats_group);
    
    // Actions group
    auto* actions_group = new QGroupBox("Cache Actions", this);
    auto* actions_layout = new QVBoxLayout(actions_group);
    
    // Clear all button
    clear_all_button_ = new QPushButton("Clear All Cache", this);
    clear_all_button_->setToolTip("Delete all cached categorization results");
    connect(clear_all_button_, &QPushButton::clicked, this, &CacheManagerDialog::on_clear_all_clicked);
    actions_layout->addWidget(clear_all_button_);
    
    // Clear old entries
    auto* clear_old_layout = new QHBoxLayout();
    clear_old_button_ = new QPushButton("Clear Entries Older Than", this);
    clear_old_button_->setToolTip("Delete cached entries older than specified days");
    connect(clear_old_button_, &QPushButton::clicked, this, &CacheManagerDialog::on_clear_old_clicked);
    
    days_spinbox_ = new QSpinBox(this);
    days_spinbox_->setRange(1, 365);
    days_spinbox_->setValue(30);
    days_spinbox_->setSuffix(" days");
    
    clear_old_layout->addWidget(clear_old_button_);
    clear_old_layout->addWidget(days_spinbox_);
    clear_old_layout->addStretch();
    actions_layout->addLayout(clear_old_layout);
    
    // Optimize database button
    optimize_button_ = new QPushButton("Optimize Database (VACUUM)", this);
    optimize_button_->setToolTip("Compact database and reclaim unused space");
    connect(optimize_button_, &QPushButton::clicked, this, &CacheManagerDialog::on_optimize_clicked);
    actions_layout->addWidget(optimize_button_);
    
    main_layout->addWidget(actions_group);
    
    main_layout->addStretch();
    
    // Bottom buttons
    auto* button_layout = new QHBoxLayout();
    
    refresh_button_ = new QPushButton("Refresh Statistics", this);
    connect(refresh_button_, &QPushButton::clicked, this, &CacheManagerDialog::on_refresh_clicked);
    button_layout->addWidget(refresh_button_);
    
    button_layout->addStretch();
    
    close_button_ = new QPushButton("Close", this);
    connect(close_button_, &QPushButton::clicked, this, &QDialog::accept);
    button_layout->addWidget(close_button_);
    
    main_layout->addLayout(button_layout);
}

void CacheManagerDialog::update_statistics() {
    auto stats = db_.get_cache_statistics();
    
    entry_count_label_->setText(QString::number(stats.entry_count));
    database_size_label_->setText(format_bytes(stats.database_size_bytes));
    folders_count_label_->setText(QString::number(stats.distinct_folders));
    oldest_entry_label_->setText(QString::fromStdString(stats.oldest_entry_date));
    newest_entry_label_->setText(QString::fromStdString(stats.newest_entry_date));
}

QString CacheManagerDialog::format_bytes(int64_t bytes) {
    const char* units[] = {"bytes", "KB", "MB", "GB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 3) {
        size /= 1024.0;
        unit_index++;
    }
    
    if (unit_index == 0) {
        return QString("%1 %2").arg(bytes).arg(units[unit_index]);
    } else {
        return QString("%1 %2").arg(size, 0, 'f', 2).arg(units[unit_index]);
    }
}

void CacheManagerDialog::on_clear_all_clicked() {
    auto reply = QMessageBox::question(
        this,
        "Confirm Clear All",
        "Are you sure you want to delete all cached categorization results?\n\n"
        "This action cannot be undone, and files will need to be re-categorized.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        if (db_.clear_all_cache()) {
            QMessageBox::information(this, "Success", "All cache cleared successfully!");
            update_statistics();
        } else {
            QMessageBox::warning(this, "Error", "Failed to clear cache. Please check the logs.");
        }
    }
}

void CacheManagerDialog::on_clear_old_clicked() {
    int days = days_spinbox_->value();
    
    auto reply = QMessageBox::question(
        this,
        "Confirm Clear Old Entries",
        QString("Are you sure you want to delete all cache entries older than %1 days?\n\n"
                "This action cannot be undone.")
            .arg(days),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        if (db_.clear_cache_older_than(days)) {
            QMessageBox::information(
                this,
                "Success",
                QString("Cache entries older than %1 days cleared successfully!").arg(days)
            );
            update_statistics();
        } else {
            QMessageBox::warning(this, "Error", "Failed to clear old cache. Please check the logs.");
        }
    }
}

void CacheManagerDialog::on_optimize_clicked() {
    QMessageBox::information(
        this,
        "Optimizing Database",
        "Database optimization started. This may take a few moments..."
    );
    
    if (db_.optimize_database()) {
        QMessageBox::information(
            this,
            "Success",
            "Database optimized successfully!\n\nUnused space has been reclaimed."
        );
        update_statistics();
    } else {
        QMessageBox::warning(
            this,
            "Error",
            "Failed to optimize database. Please check the logs."
        );
    }
}

void CacheManagerDialog::on_refresh_clicked() {
    update_statistics();
    QMessageBox::information(this, "Refreshed", "Statistics updated!");
}
