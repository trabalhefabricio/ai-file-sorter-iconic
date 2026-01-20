#ifndef FILETINDERDIALOG_HPP
#define FILETINDERDIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QKeyEvent>
#include <vector>
#include <string>
#include "DatabaseManager.hpp"

/**
 * @brief File Tinder - Swipe-style file cleanup tool
 * 
 * Presents files one at a time with options to:
 * - Keep (→ Right Arrow key)
 * - Delete (← Left Arrow key)
 * - Ignore (↓ Down Arrow key)
 * - Revert last decision (↑ Up Arrow key)
 * 
 * At the end, shows a review of all marked deletions before executing.
 */
class FileTinderDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(FileTinderDialog)

public:
    explicit FileTinderDialog(const std::string& folder_path,
                             DatabaseManager& db,
                             QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void on_keep_file();
    void on_delete_file();
    void on_ignore_file();
    void on_revert_decision();
    void on_finish_review();
    void on_execute_deletions();

private:
    enum class Decision {
        Pending,
        Keep,
        Delete,
        Ignore
    };

    struct FileToReview {
        std::string path;
        Decision decision{Decision::Pending};
        std::string file_name;
        int64_t file_size{0};
        std::string file_type;
    };

    void setup_ui();
    void load_files();
    void show_current_file();
    void preview_file(const std::string& path);
    void save_state();
    void load_state();
    void move_to_next_file();
    void update_progress();
    void show_review_screen();
    QString get_decision_icon(Decision decision) const;
    QString get_decision_text(Decision decision) const;
    QString format_file_size(int64_t bytes) const;
    
    std::vector<FileToReview> files_;
    size_t current_index_{0};
    DatabaseManager& db_;
    std::string folder_path_;
    
    // UI components
    QLabel* preview_area_;
    QLabel* file_info_label_;
    QLabel* file_name_label_;
    QLabel* stats_summary_label_;
    QProgressBar* progress_bar_;
    
    QPushButton* keep_button_;
    QPushButton* delete_button_;
    QPushButton* ignore_button_;
    QPushButton* revert_button_;
    QPushButton* finish_button_;
};

#endif // FILETINDERDIALOG_HPP
