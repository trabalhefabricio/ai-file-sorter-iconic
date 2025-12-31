#ifndef CACHEMANAGERDIALOG_HPP
#define CACHEMANAGERDIALOG_HPP

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QGroupBox>
#include <QVBoxLayout>

class DatabaseManager;

class CacheManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CacheManagerDialog(DatabaseManager& db, QWidget* parent = nullptr);
    ~CacheManagerDialog() override = default;

private slots:
    void on_clear_all_clicked();
    void on_clear_old_clicked();
    void on_optimize_clicked();
    void on_refresh_clicked();

private:
    void setup_ui();
    void update_statistics();
    QString format_bytes(int64_t bytes);
    
    DatabaseManager& db_;
    
    // UI elements
    QLabel* entry_count_label_;
    QLabel* database_size_label_;
    QLabel* oldest_entry_label_;
    QLabel* newest_entry_label_;
    QLabel* folders_count_label_;
    
    QPushButton* clear_all_button_;
    QPushButton* clear_old_button_;
    QPushButton* optimize_button_;
    QPushButton* refresh_button_;
    QPushButton* close_button_;
    
    QSpinBox* days_spinbox_;
};

#endif // CACHEMANAGERDIALOG_HPP
