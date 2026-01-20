#include "FileTinderDialog.hpp"
#include "Logger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QDir>
#include <QPixmap>
#include <QMessageBox>
#include <QScrollArea>
#include <QTextEdit>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QLocale>
#include <algorithm>

FileTinderDialog::FileTinderDialog(const std::string& folder_path,
                                   DatabaseManager& db,
                                   QWidget* parent)
    : QDialog(parent), db_(db), folder_path_(folder_path) {
    
    setWindowTitle(tr("File Tinder - Quick Cleanup"));
    setMinimumSize(800, 600);
    
    setup_ui();
    load_files();
    load_state();  // Resume from last session if exists
    show_current_file();
}

void FileTinderDialog::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    
    // Statistics summary at top
    stats_summary_label_ = new QLabel(this);
    stats_summary_label_->setAlignment(Qt::AlignCenter);
    QFont stats_font;
    stats_font.setPointSize(11);
    stats_summary_label_->setFont(stats_font);
    stats_summary_label_->setStyleSheet("QLabel { color: #555; padding: 5px; }");
    main_layout->addWidget(stats_summary_label_);
    
    // File name label at top
    file_name_label_ = new QLabel(this);
    QFont title_font;
    title_font.setPointSize(14);
    title_font.setBold(true);
    file_name_label_->setFont(title_font);
    file_name_label_->setWordWrap(true);
    file_name_label_->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(file_name_label_);
    
    // File info (size, type)
    file_info_label_ = new QLabel(this);
    file_info_label_->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(file_info_label_);
    
    // Preview area
    auto* preview_scroll = new QScrollArea(this);
    preview_scroll->setWidgetResizable(true);
    preview_scroll->setMinimumHeight(300);
    
    // BUG FIX #1: Set parent to prevent memory leak
    preview_area_ = new QLabel(preview_scroll);
    preview_area_->setAlignment(Qt::AlignCenter);
    preview_area_->setWordWrap(true);
    preview_area_->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 20px; }");
    preview_scroll->setWidget(preview_area_);
    
    main_layout->addWidget(preview_scroll, 1);  // Stretch factor 1
    
    // Progress bar with better styling
    progress_bar_ = new QProgressBar(this);
    progress_bar_->setTextVisible(true);
    progress_bar_->setFormat(tr("Progress: %v / %m files (%p%)"));
    progress_bar_->setMinimumHeight(30);
    QFont progress_font;
    progress_font.setPointSize(10);
    progress_bar_->setFont(progress_font);
    progress_bar_->setStyleSheet(
        "QProgressBar {"
        "    border: 2px solid #ccc;"
        "    border-radius: 5px;"
        "    text-align: center;"
        "    background-color: #f0f0f0;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #4CAF50;"
        "    border-radius: 3px;"
        "}"
    );
    main_layout->addWidget(progress_bar_);
    
    // Button layout
    auto* button_layout = new QHBoxLayout();
    
    // Revert button (left side) - Up Arrow
    revert_button_ = new QPushButton(tr("↑ Back"), this);
    revert_button_->setToolTip(tr("Undo last decision (↑ Up Arrow key)"));
    revert_button_->setStyleSheet("QPushButton { font-size: 16px; padding: 15px 30px; }");
    button_layout->addWidget(revert_button_);
    
    button_layout->addStretch();
    
    // Main action buttons
    delete_button_ = new QPushButton(tr("← Delete"), this);
    delete_button_->setStyleSheet("QPushButton { background-color: #ef5350; color: white; font-size: 16px; padding: 15px 30px; }");
    delete_button_->setToolTip(tr("Mark for deletion (← Left Arrow key)"));
    button_layout->addWidget(delete_button_);
    
    ignore_button_ = new QPushButton(tr("↓ Skip"), this);
    ignore_button_->setStyleSheet("QPushButton { font-size: 16px; padding: 15px 30px; }");
    ignore_button_->setToolTip(tr("Ignore this file (↓ Down Arrow key)"));
    button_layout->addWidget(ignore_button_);
    
    keep_button_ = new QPushButton(tr("→ Keep"), this);
    keep_button_->setStyleSheet("QPushButton { background-color: #66bb6a; color: white; font-size: 16px; padding: 15px 30px; }");
    keep_button_->setToolTip(tr("Keep this file (→ Right Arrow key)"));
    button_layout->addWidget(keep_button_);
    
    button_layout->addStretch();
    
    // Finish button (right side)
    finish_button_ = new QPushButton(tr("Finish Review →"), this);
    finish_button_->setToolTip(tr("Complete review and see results"));
    button_layout->addWidget(finish_button_);
    
    main_layout->addLayout(button_layout);
    
    // Connect signals
    connect(keep_button_, &QPushButton::clicked, this, &FileTinderDialog::on_keep_file);
    connect(delete_button_, &QPushButton::clicked, this, &FileTinderDialog::on_delete_file);
    connect(ignore_button_, &QPushButton::clicked, this, &FileTinderDialog::on_ignore_file);
    connect(revert_button_, &QPushButton::clicked, this, &FileTinderDialog::on_revert_decision);
    connect(finish_button_, &QPushButton::clicked, this, &FileTinderDialog::on_finish_review);
}

void FileTinderDialog::load_files() {
    QDir dir(QString::fromStdString(folder_path_));
    if (!dir.exists()) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("Folder does not exist: {}", folder_path_);
        }
        // Initialize progress bar to show "empty" state (not indeterminate)
        progress_bar_->setMaximum(1);
        progress_bar_->setValue(0);
        return;
    }
    
    // Get all files (non-recursive for now)
    QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    
    for (const QFileInfo& file_info : file_list) {
        FileToReview file;
        file.path = file_info.absoluteFilePath().toStdString();
        file.file_name = file_info.fileName().toStdString();
        file.file_size = file_info.size();
        file.file_type = file_info.suffix().toStdString();
        file.decision = Decision::Pending;
        
        files_.push_back(file);
    }
    
    progress_bar_->setMaximum(files_.size() > 0 ? files_.size() : 1);
    progress_bar_->setValue(0);
    
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->info("Loaded {} files for File Tinder review", files_.size());
    }
}

void FileTinderDialog::show_current_file() {
    if (current_index_ >= files_.size()) {
        // All files reviewed
        show_review_screen();
        return;
    }
    
    const auto& file = files_[current_index_];
    
    // Update file name
    file_name_label_->setText(QString::fromStdString(file.file_name));
    
    // Update file info
    QString info = tr("Size: %1 | Type: %2")
        .arg(format_file_size(file.file_size))
        .arg(QString::fromStdString(file.file_type).isEmpty() ? tr("(no extension)") : QString::fromStdString(file.file_type));
    file_info_label_->setText(info);
    
    // Preview file
    preview_file(file.path);
    
    // Update progress
    update_progress();
    
    // Enable/disable revert button
    revert_button_->setEnabled(current_index_ > 0);
}

void FileTinderDialog::preview_file(const std::string& path) {
    QFileInfo file_info(QString::fromStdString(path));
    QString suffix = file_info.suffix().toLower();
    
    // BUG FIX #11: Add specific error logging for file operations
    // Image preview
    if (suffix == "jpg" || suffix == "jpeg" || suffix == "png" || 
        suffix == "gif" || suffix == "bmp" || suffix == "webp") {
        
        QPixmap pixmap(QString::fromStdString(path));
        if (!pixmap.isNull()) {
            // Scale to fit preview area while maintaining aspect ratio
            QPixmap scaled = pixmap.scaled(700, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            preview_area_->setPixmap(scaled);
            preview_area_->setText("");
        } else {
            preview_area_->clear();
            preview_area_->setText(tr("Unable to load image"));
            // BUG FIX #11: Add specific error logging
            if (auto logger = Logger::get_logger("core_logger")) {
                logger->warn("Failed to load image preview for file: {} (format: {})", path, suffix.toStdString());
            }
        }
    }
    // Text file preview
    else if (suffix == "txt" || suffix == "log" || suffix == "md" || 
             suffix == "json" || suffix == "xml" || suffix == "csv" ||
             suffix == "cpp" || suffix == "h" || suffix == "py" || suffix == "js") {
        
        QFile file(QString::fromStdString(path));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.read(2000);  // Read first 2000 characters
            if (!in.atEnd()) {
                content += "\n\n... (truncated)";
            }
            preview_area_->clear();
            preview_area_->setText(content);
            preview_area_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        } else {
            preview_area_->clear();
            preview_area_->setText(tr("Unable to read file"));
            // BUG FIX #11: Add specific error logging with error details
            if (auto logger = Logger::get_logger("core_logger")) {
                logger->warn("Failed to open text file for preview: {} (error: {})", 
                           path, file.errorString().toStdString());
            }
        }
    }
    // Default: show file info
    else {
        preview_area_->clear();
        QString info = tr("File: %1\n\n"
                         "Size: %2\n"
                         "Type: %3\n"
                         "Modified: %4\n\n"
                         "(Preview not available for this file type)")
            .arg(file_info.fileName())
            .arg(format_file_size(file_info.size()))
            .arg(suffix.isEmpty() ? tr("(no extension)") : suffix)
            .arg(QLocale::system().toString(file_info.lastModified()));
        preview_area_->setText(info);
        preview_area_->setAlignment(Qt::AlignCenter);
    }
}

void FileTinderDialog::on_keep_file() {
    if (current_index_ >= files_.size()) return;
    
    files_[current_index_].decision = Decision::Keep;
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->debug("Marked file as KEEP: {}", files_[current_index_].file_name);
    }
    
    save_state();
    move_to_next_file();
}

void FileTinderDialog::on_delete_file() {
    if (current_index_ >= files_.size()) return;
    
    files_[current_index_].decision = Decision::Delete;
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->debug("Marked file as DELETE: {}", files_[current_index_].file_name);
    }
    
    save_state();
    move_to_next_file();
}

void FileTinderDialog::on_ignore_file() {
    if (current_index_ >= files_.size()) return;
    
    files_[current_index_].decision = Decision::Ignore;
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->debug("Marked file as IGNORE: {}", files_[current_index_].file_name);
    }
    
    save_state();
    move_to_next_file();
}

void FileTinderDialog::on_revert_decision() {
    if (current_index_ == 0) return;
    
    // Go back to previous file
    current_index_--;
    
    // Reset its decision to pending
    files_[current_index_].decision = Decision::Pending;
    
    if (auto logger = Logger::get_logger("core_logger")) {
        logger->debug("Reverted decision for: {}", files_[current_index_].file_name);
    }
    
    save_state();
    show_current_file();
}

void FileTinderDialog::move_to_next_file() {
    current_index_++;
    show_current_file();
}

void FileTinderDialog::update_progress() {
    progress_bar_->setValue(current_index_);
    
    // Update statistics summary
    int keep_count = 0;
    int delete_count = 0;
    int ignore_count = 0;
    
    for (const auto& file : files_) {
        switch (file.decision) {
            case Decision::Keep: keep_count++; break;
            case Decision::Delete: delete_count++; break;
            case Decision::Ignore: ignore_count++; break;
            default: break;
        }
    }
    
    stats_summary_label_->setText(
        tr("✓ Keep: %1  |  ✗ Delete: %2  |  ↓ Skip: %3")
            .arg(keep_count)
            .arg(delete_count)
            .arg(ignore_count)
    );
}

void FileTinderDialog::on_finish_review() {
    show_review_screen();
}

void FileTinderDialog::show_review_screen() {
    // Count decisions
    int keep_count = 0;
    int delete_count = 0;
    int ignore_count = 0;
    int pending_count = 0;
    
    for (const auto& file : files_) {
        switch (file.decision) {
            case Decision::Keep: keep_count++; break;
            case Decision::Delete: delete_count++; break;
            case Decision::Ignore: ignore_count++; break;
            case Decision::Pending: pending_count++; break;
        }
    }
    
    // Show review dialog
    QString message = tr("Review Summary:\n\n"
                        "Keep: %1 files\n"
                        "Delete: %2 files\n"
                        "Ignore: %3 files\n"
                        "Pending: %4 files\n\n")
        .arg(keep_count)
        .arg(delete_count)
        .arg(ignore_count)
        .arg(pending_count);
    
    if (delete_count == 0) {
        message += tr("No files marked for deletion.");
        QMessageBox::information(this, tr("Review Complete"), message);
        accept();
        return;
    }
    
    message += tr("Do you want to PERMANENTLY DELETE %1 files?").arg(delete_count);
    
    auto reply = QMessageBox::warning(this, tr("Confirm Deletion"), message,
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    
    if (reply == QMessageBox::Yes) {
        on_execute_deletions();
    } else if (reply == QMessageBox::No) {
        // Just close without deleting
        accept();
    }
    // Cancel = stay in dialog
}

void FileTinderDialog::on_execute_deletions() {
    int deleted_count = 0;
    int failed_count = 0;
    int kept_count = 0;
    QString error_messages;
    
    // BUG FIX #5: Wrap file operations in try-catch for exception safety
    try {
        for (const auto& file : files_) {
            if (file.decision == Decision::Delete) {
                try {
                    QFile qfile(QString::fromStdString(file.path));
                    if (qfile.remove()) {
                        deleted_count++;
                        if (auto logger = Logger::get_logger("core_logger")) {
                            logger->info("Deleted file: {}", file.path);
                        }
                    } else {
                        failed_count++;
                        QString error = tr("Failed to delete: %1 - %2\n")
                            .arg(QString::fromStdString(file.file_name))
                            .arg(qfile.errorString());
                        error_messages += error;
                        if (auto logger = Logger::get_logger("core_logger")) {
                            logger->error("Failed to delete file: {} - {}", file.path, 
                                        qfile.errorString().toStdString());
                        }
                    }
                } catch (const std::exception& e) {
                    failed_count++;
                    QString error = tr("Exception deleting: %1 - %2\n")
                        .arg(QString::fromStdString(file.file_name))
                        .arg(e.what());
                    error_messages += error;
                    if (auto logger = Logger::get_logger("core_logger")) {
                        logger->error("Exception deleting file: {} - {}", file.path, e.what());
                    }
                }
            } else if (file.decision == Decision::Keep) {
                kept_count++;
            }
        }
    } catch (const std::exception& e) {
        // BUG FIX #5: Catch any exception during the deletion loop
        QMessageBox::critical(this, tr("Error"), 
            tr("Critical error during deletion: %1").arg(e.what()));
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("Critical error during file deletions: {}", e.what());
        }
        return;
    }
    
    // BUG FIX #6: Check return value of clear_tinder_session and show error if it fails
    bool session_cleared = db_.clear_tinder_session(folder_path_);
    
    // Show results
    QString result_message = tr("Deletion complete:\n\n"
                                "Successfully deleted: %1 files\n"
                                "Kept: %2 files\n"
                                "Failed: %3 files")
        .arg(deleted_count)
        .arg(kept_count)
        .arg(failed_count);
    
    if (failed_count > 0) {
        result_message += "\n\n" + tr("Errors:\n") + error_messages;
    }
    
    // Add warning about session clearing failure if it occurred
    if (!session_cleared) {
        result_message += "\n\n" + tr("Warning: Failed to clear session data.\n"
                                      "Previous tinder decisions may still appear on next session.");
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("Failed to clear tinder session for folder: {}", folder_path_);
        }
    }
    
    // Show appropriate message box based on whether there were errors
    if (failed_count > 0 || !session_cleared) {
        QMessageBox::warning(this, tr("Deletion Results"), result_message);
    } else {
        QMessageBox::information(this, tr("Deletion Results"), result_message);
    }
    
    accept();
}

void FileTinderDialog::save_state() {
    // Save current state to database for session resumption
    bool any_save_failed = false;
    
    for (const auto& file : files_) {
        if (file.decision == Decision::Pending) continue;
        
        DatabaseManager::FileTinderDecision decision;
        decision.folder_path = folder_path_;
        decision.file_path = file.path;
        
        switch (file.decision) {
            case Decision::Keep: decision.decision = "keep"; break;
            case Decision::Delete: decision.decision = "delete"; break;
            case Decision::Ignore: decision.decision = "ignore"; break;
            default: decision.decision = "pending"; break;
        }
        
        if (!db_.save_tinder_decision(decision)) {
            any_save_failed = true;
            if (auto logger = Logger::get_logger("core_logger")) {
                logger->warn("Failed to save tinder decision for file: {} (folder: {}, decision: {})", 
                           file.path, folder_path_, decision.decision);
            }
        }
    }
    
    // Only notify user if saves failed (don't interrupt workflow for background saves)
    if (any_save_failed) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("Some tinder decisions failed to save to database");
        }
    }
}

void FileTinderDialog::load_state() {
    // Load previous session state if exists
    auto decisions = db_.get_tinder_decisions(folder_path_);
    
    for (const auto& db_decision : decisions) {
        // Find matching file
        for (auto& file : files_) {
            if (file.path == db_decision.file_path) {
                if (db_decision.decision == "keep") {
                    file.decision = Decision::Keep;
                } else if (db_decision.decision == "delete") {
                    file.decision = Decision::Delete;
                } else if (db_decision.decision == "ignore") {
                    file.decision = Decision::Ignore;
                }
                break;
            }
        }
    }
    
    // BUG FIX #10: Add bounds checking before accessing files_ vector
    // Find first pending file
    for (size_t i = 0; i < files_.size(); ++i) {
        if (files_[i].decision == Decision::Pending) {
            current_index_ = i;
            break;
        }
    }
    
    // BUG FIX #10: Explicit bounds check with size_t comparison
    if (!files_.empty() && current_index_ >= files_.size()) {
        current_index_ = files_.size() - 1;
    } else if (files_.empty()) {
        current_index_ = 0;
    }
}

void FileTinderDialog::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Right:  // → Keep
            on_keep_file();
            break;
        case Qt::Key_Left:   // ← Delete
            on_delete_file();
            break;
        case Qt::Key_Down:   // ↓ Skip/Ignore
            on_ignore_file();
            break;
        case Qt::Key_Up:     // ↑ Back/Revert
            on_revert_decision();
            break;
        default:
            QDialog::keyPressEvent(event);
    }
}

QString FileTinderDialog::format_file_size(int64_t bytes) const {
    // BUG FIX #6: Prevent integer overflow by using explicit constants and safe casts
    const int64_t KB = 1024LL;
    const int64_t MB = 1024LL * KB;
    const int64_t GB = 1024LL * MB;
    
    // Handle negative values
    if (bytes < 0) {
        return QString("0 bytes");
    }
    
    if (bytes >= GB) {
        double gb_value = static_cast<double>(bytes) / static_cast<double>(GB);
        return QString("%1 GB").arg(gb_value, 0, 'f', 2);
    } else if (bytes >= MB) {
        double mb_value = static_cast<double>(bytes) / static_cast<double>(MB);
        return QString("%1 MB").arg(mb_value, 0, 'f', 2);
    } else if (bytes >= KB) {
        double kb_value = static_cast<double>(bytes) / static_cast<double>(KB);
        return QString("%1 KB").arg(kb_value, 0, 'f', 1);
    }
    return QString("%1 bytes").arg(bytes);
}

QString FileTinderDialog::get_decision_icon(Decision decision) const {
    switch (decision) {
        case Decision::Keep: return "✓";
        case Decision::Delete: return "✗";
        case Decision::Ignore: return "↓";
        default: return "?";
    }
}

QString FileTinderDialog::get_decision_text(Decision decision) const {
    switch (decision) {
        case Decision::Keep: return tr("Keep");
        case Decision::Delete: return tr("Delete");
        case Decision::Ignore: return tr("Ignore");
        default: return tr("Pending");
    }
}
