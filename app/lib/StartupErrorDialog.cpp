#include "StartupErrorDialog.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QSysInfo>
#include <QDateTime>
#include <QFont>
#include <QFontDatabase>

#include <filesystem>
#include <sstream>

StartupErrorDialog::StartupErrorDialog(const std::string& error_message,
                                       const std::string& error_details,
                                       QWidget* parent)
    : QDialog(parent),
      error_message_(QString::fromStdString(error_message)),
      error_details_(QString::fromStdString(error_details))
{
    try {
        log_directory_ = QString::fromStdString(Logger::get_log_directory());
    } catch (...) {
        log_directory_ = tr("Unable to determine log directory");
    }
    
    setup_ui();
    setMinimumSize(700, 500);
    setWindowTitle(tr("AI File Sorter - Startup Error"));
}

void StartupErrorDialog::setup_ui()
{
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(15);
    main_layout->setContentsMargins(20, 20, 20, 20);

    // Error icon and title
    auto* header_layout = new QHBoxLayout();
    auto* icon_label = new QLabel(this);
    icon_label->setPixmap(style()->standardPixmap(QStyle::SP_MessageBoxCritical));
    header_layout->addWidget(icon_label);
    
    auto* title_label = new QLabel(tr("Application Startup Failed"), this);
    QFont title_font = title_label->font();
    title_font.setBold(true);
    title_font.setPointSize(title_font.pointSize() + 2);
    title_label->setFont(title_font);
    header_layout->addWidget(title_label);
    header_layout->addStretch();
    
    main_layout->addLayout(header_layout);

    // Explanation text
    auto* explanation = new QLabel(
        tr("AI File Sorter encountered an error during startup and cannot continue.\n"
           "The information below can help diagnose the problem."),
        this
    );
    explanation->setWordWrap(true);
    main_layout->addWidget(explanation);

    // Error details text area
    text_display_ = new QTextEdit(this);
    text_display_->setReadOnly(true);
    text_display_->setPlainText(generate_error_report());
    // Use system's monospace font for better cross-platform compatibility
    text_display_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    main_layout->addWidget(text_display_);

    // Action buttons
    auto* button_layout = new QHBoxLayout();
    
    copy_button_ = new QPushButton(tr("Copy to Clipboard"), this);
    copy_button_->setToolTip(tr("Copy all error information for bug reports"));
    connect(copy_button_, &QPushButton::clicked, this, &StartupErrorDialog::copy_to_clipboard);
    button_layout->addWidget(copy_button_);
    
    open_logs_button_ = new QPushButton(tr("Open Log Folder"), this);
    open_logs_button_->setToolTip(tr("Open the folder containing log files"));
    connect(open_logs_button_, &QPushButton::clicked, this, &StartupErrorDialog::open_log_directory);
    button_layout->addWidget(open_logs_button_);
    
    button_layout->addStretch();
    
    close_button_ = new QPushButton(tr("Close"), this);
    connect(close_button_, &QPushButton::clicked, this, &QDialog::accept);
    button_layout->addWidget(close_button_);
    
    main_layout->addLayout(button_layout);

    // Help text
    auto* help_label = new QLabel(
        tr("💡 <b>What to do:</b><br>"
           "• Copy the error information and report it to the developers<br>"
           "• Check the log files for more details<br>"
           "• Verify you have proper permissions to access application folders"),
        this
    );
    help_label->setWordWrap(true);
    help_label->setStyleSheet("QLabel { background-color: #f0f8ff; padding: 10px; border-radius: 5px; }");
    main_layout->addWidget(help_label);
}

QString StartupErrorDialog::generate_error_report() const
{
    std::ostringstream report;
    
    report << "=== AI File Sorter STARTUP ERROR REPORT ===" << std::endl;
    report << "Timestamp: " << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << std::endl;
    report << std::endl;
    
    report << "--- Error Information ---" << std::endl;
    report << "Error: " << error_message_.toStdString() << std::endl;
    if (!error_details_.isEmpty()) {
        report << "Details: " << error_details_.toStdString() << std::endl;
    }
    report << std::endl;
    
    report << "--- Log File Locations ---" << std::endl;
    report << "Log Directory: " << log_directory_.toStdString() << std::endl;
    
    // List actual log files if directory exists
    try {
        std::filesystem::path log_path(log_directory_.toStdString());
        if (std::filesystem::exists(log_path) && std::filesystem::is_directory(log_path)) {
            report << "Available log files:" << std::endl;
            for (const auto& entry : std::filesystem::directory_iterator(log_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".log") {
                    report << "  - " << entry.path().filename().string() << std::endl;
                }
            }
        } else {
            report << "  (Log directory does not exist or is not accessible)" << std::endl;
        }
    } catch (const std::exception& e) {
        report << "  (Error accessing log directory: " << e.what() << ")" << std::endl;
    }
    report << std::endl;
    
    report << get_system_info().toStdString();
    
    report << std::endl;
    report << "--- Troubleshooting Steps ---" << std::endl;
    report << "1. Check that you have write permissions to the log directory" << std::endl;
    report << "2. Ensure required dependencies are installed (Qt6, libcurl, etc.)" << std::endl;
    report << "3. Verify the application config directory is accessible" << std::endl;
    report << "4. Check log files for more detailed error messages" << std::endl;
    report << "5. Try running the application from the command line with --console-log" << std::endl;
    report << std::endl;
    
    report << "Please report this error at:" << std::endl;
    report << "https://github.com/hyperfield/ai-file-sorter/issues" << std::endl;
    
    return QString::fromStdString(report.str());
}

QString StartupErrorDialog::get_system_info() const
{
    std::ostringstream info;
    
    info << "--- System Information ---" << std::endl;
    info << "OS: " << QSysInfo::prettyProductName().toStdString() << std::endl;
    info << "Architecture: " << QSysInfo::currentCpuArchitecture().toStdString() << std::endl;
    info << "Kernel: " << QSysInfo::kernelType().toStdString() 
         << " " << QSysInfo::kernelVersion().toStdString() << std::endl;
    info << "Qt Version: " << QT_VERSION_STR << std::endl;
    
    // Try to get executable path
    try {
        std::string exe_path = Utils::get_executable_path();
        info << "Executable: " << exe_path << std::endl;
    } catch (...) {
        info << "Executable: (unable to determine)" << std::endl;
    }
    
    return QString::fromStdString(info.str());
}

void StartupErrorDialog::copy_to_clipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    if (clipboard) {
        clipboard->setText(generate_error_report());
        QMessageBox::information(
            this,
            tr("Copied"),
            tr("Error information has been copied to clipboard.\n"
               "You can paste it into a bug report or support request.")
        );
    }
}

void StartupErrorDialog::open_log_directory()
{
    if (log_directory_.isEmpty() || log_directory_ == tr("Unable to determine log directory")) {
        QMessageBox::warning(
            this,
            tr("Cannot Open"),
            tr("Log directory location is not available.")
        );
        return;
    }
    
    // Try to create the directory if it doesn't exist
    try {
        std::filesystem::path log_path(log_directory_.toStdString());
        if (!std::filesystem::exists(log_path)) {
            std::filesystem::create_directories(log_path);
        }
    } catch (...) {
        // Ignore errors, just try to open
    }
    
    QUrl url = QUrl::fromLocalFile(log_directory_);
    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(
            this,
            tr("Cannot Open"),
            tr("Failed to open log directory:\n%1\n\n"
               "Please navigate to this location manually.")
               .arg(log_directory_)
        );
    }
}

void StartupErrorDialog::show_startup_error(const std::string& error_message,
                                           const std::string& error_details,
                                           QWidget* parent)
{
    StartupErrorDialog dialog(error_message, error_details, parent);
    dialog.exec();
}
