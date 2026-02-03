#ifndef CACHEMANAGERDIALOG_HPP
#define CACHEMANAGERDIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QProgressBar>
#include "DatabaseManager.hpp"

/**
 * @brief Cache Management Dialog
 * 
 * Provides a user interface for managing the categorization cache:
 * - View cache statistics (entry count, size, dates)
 * - Clear all cache with confirmation
 * - Clear cache older than N days
 * - Optimize database (VACUUM) to reclaim space
 * - Real-time statistics refresh
 */
class CacheManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CacheManagerDialog(DatabaseManager& db, QWidget* parent = nullptr);

private slots:
    void on_refresh_stats();
    void on_clear_all_cache();
    void on_clear_old_cache();
    void on_optimize_database();

private:
    void setup_ui();
    void update_stats_display(const DatabaseManager::CacheStats& stats);
    QString format_file_size(int64_t bytes) const;
    void set_buttons_enabled(bool enabled);
    void show_operation_in_progress(const QString& operation);
    void hide_progress();

    DatabaseManager& db_;

    // Stats display labels
    QLabel* entry_count_label_;
    QLabel* db_size_label_;
    QLabel* oldest_entry_label_;
    QLabel* newest_entry_label_;
    QLabel* taxonomy_count_label_;
    QLabel* db_path_label_;

    // Action buttons
    QPushButton* refresh_btn_;
    QPushButton* clear_all_btn_;
    QPushButton* clear_old_btn_;
    QPushButton* optimize_btn_;
    QPushButton* close_btn_;

    // Controls
    QSpinBox* days_spinbox_;
    
    // Progress indicator
    QProgressBar* progress_bar_;
    QLabel* status_label_;
};

#endif // CACHEMANAGERDIALOG_HPP
