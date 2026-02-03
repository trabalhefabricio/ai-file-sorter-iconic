#include "CacheManagerDialog.hpp"
#include "Logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>
#include <QStyle>

namespace {
// File size formatting constants
constexpr int64_t kBytesPerKB = 1024LL;
constexpr int64_t kBytesPerMB = 1024LL * kBytesPerKB;
constexpr int64_t kBytesPerGB = 1024LL * kBytesPerMB;
} // namespace

CacheManagerDialog::CacheManagerDialog(DatabaseManager& db, QWidget* parent)
    : QDialog(parent), db_(db) {
    
    setWindowTitle(tr("Cache Management"));
    setMinimumWidth(500);
    resize(550, 400);
    
    setup_ui();
    on_refresh_stats();  // Initial stats load
}

void CacheManagerDialog::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);

    // Statistics Group
    auto* stats_group = new QGroupBox(tr("Cache Statistics"), this);
    auto* stats_layout = new QFormLayout(stats_group);

    entry_count_label_ = new QLabel(tr("Loading..."), this);
    db_size_label_ = new QLabel(tr("Loading..."), this);
    oldest_entry_label_ = new QLabel(tr("Loading..."), this);
    newest_entry_label_ = new QLabel(tr("Loading..."), this);
    taxonomy_count_label_ = new QLabel(tr("Loading..."), this);
    db_path_label_ = new QLabel(this);
    db_path_label_->setWordWrap(true);
    db_path_label_->setStyleSheet("QLabel { color: #666; font-size: 10px; }");

    stats_layout->addRow(tr("Cached Entries:"), entry_count_label_);
    stats_layout->addRow(tr("Database Size:"), db_size_label_);
    stats_layout->addRow(tr("Oldest Entry:"), oldest_entry_label_);
    stats_layout->addRow(tr("Newest Entry:"), newest_entry_label_);
    stats_layout->addRow(tr("Taxonomy Entries:"), taxonomy_count_label_);
    stats_layout->addRow(tr("Database Path:"), db_path_label_);

    main_layout->addWidget(stats_group);

    // Actions Group
    auto* actions_group = new QGroupBox(tr("Cache Actions"), this);
    auto* actions_layout = new QVBoxLayout(actions_group);

    // Refresh button
    refresh_btn_ = new QPushButton(tr("Refresh Statistics"), this);
    refresh_btn_->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(refresh_btn_, &QPushButton::clicked, this, &CacheManagerDialog::on_refresh_stats);
    actions_layout->addWidget(refresh_btn_);

    // Clear all cache button
    clear_all_btn_ = new QPushButton(tr("Clear All Cache"), this);
    clear_all_btn_->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    clear_all_btn_->setToolTip(tr("Delete all cached categorization results"));
    connect(clear_all_btn_, &QPushButton::clicked, this, &CacheManagerDialog::on_clear_all_cache);
    actions_layout->addWidget(clear_all_btn_);

    // Clear old cache row
    auto* clear_old_layout = new QHBoxLayout();
    clear_old_btn_ = new QPushButton(tr("Clear Entries Older Than:"), this);
    clear_old_btn_->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    clear_old_btn_->setToolTip(tr("Delete cache entries older than the specified number of days"));
    connect(clear_old_btn_, &QPushButton::clicked, this, &CacheManagerDialog::on_clear_old_cache);
    
    days_spinbox_ = new QSpinBox(this);
    days_spinbox_->setRange(1, 365);
    days_spinbox_->setValue(30);
    days_spinbox_->setSuffix(tr(" days"));
    days_spinbox_->setMinimumWidth(100);

    clear_old_layout->addWidget(clear_old_btn_);
    clear_old_layout->addWidget(days_spinbox_);
    clear_old_layout->addStretch();
    actions_layout->addLayout(clear_old_layout);

    // Optimize database button
    optimize_btn_ = new QPushButton(tr("Optimize Database (Reclaim Space)"), this);
    optimize_btn_->setIcon(style()->standardIcon(QStyle::SP_DriveHDIcon));
    optimize_btn_->setToolTip(tr("Run VACUUM to reclaim unused space in the database file"));
    connect(optimize_btn_, &QPushButton::clicked, this, &CacheManagerDialog::on_optimize_database);
    actions_layout->addWidget(optimize_btn_);

    main_layout->addWidget(actions_group);

    // Progress area
    auto* progress_layout = new QHBoxLayout();
    progress_bar_ = new QProgressBar(this);
    progress_bar_->setRange(0, 0);  // Indeterminate
    progress_bar_->setVisible(false);
    progress_bar_->setMinimumWidth(200);

    status_label_ = new QLabel(this);
    status_label_->setVisible(false);

    progress_layout->addWidget(progress_bar_);
    progress_layout->addWidget(status_label_);
    progress_layout->addStretch();
    main_layout->addLayout(progress_layout);

    // Add stretch to push close button to bottom
    main_layout->addStretch();

    // Close button
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    close_btn_ = new QPushButton(tr("Close"), this);
    connect(close_btn_, &QPushButton::clicked, this, &QDialog::accept);
    button_layout->addWidget(close_btn_);
    main_layout->addLayout(button_layout);
}

void CacheManagerDialog::on_refresh_stats() {
    show_operation_in_progress(tr("Refreshing statistics..."));
    QApplication::processEvents();

    auto stats = db_.get_cache_stats();
    update_stats_display(stats);

    hide_progress();
}

void CacheManagerDialog::update_stats_display(const DatabaseManager::CacheStats& stats) {
    entry_count_label_->setText(QString::number(stats.entry_count));
    db_size_label_->setText(format_file_size(stats.database_size_bytes));
    
    if (stats.oldest_entry_date.empty()) {
        oldest_entry_label_->setText(tr("No entries"));
    } else {
        oldest_entry_label_->setText(QString::fromStdString(stats.oldest_entry_date));
    }
    
    if (stats.newest_entry_date.empty()) {
        newest_entry_label_->setText(tr("No entries"));
    } else {
        newest_entry_label_->setText(QString::fromStdString(stats.newest_entry_date));
    }
    
    taxonomy_count_label_->setText(QString::number(stats.taxonomy_entry_count));
    db_path_label_->setText(QString::fromStdString(db_.get_database_path()));
}

void CacheManagerDialog::on_clear_all_cache() {
    auto reply = QMessageBox::warning(this,
                                      tr("Clear All Cache"),
                                      tr("This will permanently delete all cached categorization results.\n\n"
                                         "Files will need to be re-analyzed the next time you scan them.\n\n"
                                         "Are you sure you want to continue?"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    set_buttons_enabled(false);
    show_operation_in_progress(tr("Clearing cache..."));
    QApplication::processEvents();

    bool success = db_.clear_all_cache();

    hide_progress();
    set_buttons_enabled(true);

    if (success) {
        QMessageBox::information(this,
                                tr("Cache Cleared"),
                                tr("All cached categorization results have been deleted."));
        on_refresh_stats();
    } else {
        QMessageBox::critical(this,
                             tr("Error"),
                             tr("Failed to clear cache. Check the log files for details."));
    }
}

void CacheManagerDialog::on_clear_old_cache() {
    int days = days_spinbox_->value();
    
    auto reply = QMessageBox::question(this,
                                       tr("Clear Old Cache"),
                                       tr("Delete cache entries older than %1 days?\n\n"
                                          "This cannot be undone.").arg(days),
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    set_buttons_enabled(false);
    show_operation_in_progress(tr("Clearing old entries..."));
    QApplication::processEvents();

    int deleted_count = db_.clear_cache_older_than(days);

    hide_progress();
    set_buttons_enabled(true);

    if (deleted_count >= 0) {
        QMessageBox::information(this,
                                tr("Old Entries Cleared"),
                                tr("Deleted %1 cache entries older than %2 days.")
                                    .arg(deleted_count).arg(days));
        on_refresh_stats();
    } else {
        QMessageBox::critical(this,
                             tr("Error"),
                             tr("Failed to clear old cache entries. Check the log files for details."));
    }
}

void CacheManagerDialog::on_optimize_database() {
    auto reply = QMessageBox::question(this,
                                       tr("Optimize Database"),
                                       tr("This will compact the database file and reclaim unused space.\n\n"
                                          "This operation may take a moment for large databases.\n\n"
                                          "Continue?"),
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    set_buttons_enabled(false);
    show_operation_in_progress(tr("Optimizing database..."));
    QApplication::processEvents();

    // Get size before optimization
    auto stats_before = db_.get_cache_stats();
    int64_t size_before = stats_before.database_size_bytes;

    bool success = db_.optimize_database();

    hide_progress();
    set_buttons_enabled(true);

    if (success) {
        // Refresh stats to show new size
        on_refresh_stats();
        auto stats_after = db_.get_cache_stats();
        int64_t size_after = stats_after.database_size_bytes;
        int64_t saved = size_before - size_after;

        QString message;
        if (saved > 0) {
            message = tr("Database optimized successfully.\n\nSpace reclaimed: %1")
                         .arg(format_file_size(saved));
        } else {
            message = tr("Database optimized successfully.\n\nNo space to reclaim (database was already compact).");
        }
        QMessageBox::information(this, tr("Optimization Complete"), message);
    } else {
        QMessageBox::critical(this,
                             tr("Error"),
                             tr("Failed to optimize database. Check the log files for details."));
    }
}

QString CacheManagerDialog::format_file_size(int64_t bytes) const {
    if (bytes < 0) {
        return QString("0 bytes");
    }

    if (bytes >= kBytesPerGB) {
        double gb_value = static_cast<double>(bytes) / static_cast<double>(kBytesPerGB);
        return QString("%1 GB").arg(gb_value, 0, 'f', 2);
    }
    if (bytes >= kBytesPerMB) {
        double mb_value = static_cast<double>(bytes) / static_cast<double>(kBytesPerMB);
        return QString("%1 MB").arg(mb_value, 0, 'f', 2);
    }
    if (bytes >= kBytesPerKB) {
        double kb_value = static_cast<double>(bytes) / static_cast<double>(kBytesPerKB);
        return QString("%1 KB").arg(kb_value, 0, 'f', 1);
    }

    return QString("%1 bytes").arg(bytes);
}

void CacheManagerDialog::set_buttons_enabled(bool enabled) {
    refresh_btn_->setEnabled(enabled);
    clear_all_btn_->setEnabled(enabled);
    clear_old_btn_->setEnabled(enabled);
    optimize_btn_->setEnabled(enabled);
    days_spinbox_->setEnabled(enabled);
}

void CacheManagerDialog::show_operation_in_progress(const QString& operation) {
    progress_bar_->setVisible(true);
    status_label_->setText(operation);
    status_label_->setVisible(true);
}

void CacheManagerDialog::hide_progress() {
    progress_bar_->setVisible(false);
    status_label_->setVisible(false);
}
