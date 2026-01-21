#include "ErrorHandler.hpp"
#include "Logger.hpp"

#include <QMessageBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QApplication>

#include <spdlog/spdlog.h>

void ErrorHandler::show_error_with_context(QWidget* parent,
                                           const QString& title,
                                           const QString& message,
                                           const QString& details,
                                           bool include_log_info)
{
    QString full_message = message;
    
    if (!details.isEmpty()) {
        full_message += "\n\n" + details;
    }
    
    if (include_log_info) {
        full_message += "\n\n" + get_log_location_message();
    }
    
    QMessageBox::critical(parent, title, full_message);
}

void ErrorHandler::show_error_with_log_access(QWidget* parent,
                                              const QString& title,
                                              const QString& message,
                                              const QString& details)
{
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    
    QString detailed_text = details;
    if (!detailed_text.isEmpty()) {
        detailed_text += "\n\n";
    }
    detailed_text += get_log_location_message();
    msgBox.setDetailedText(detailed_text);
    
    // Add custom buttons
    QPushButton* openLogsButton = msgBox.addButton(
        QObject::tr("Open Log Folder"), 
        QMessageBox::ActionRole
    );
    QPushButton* copyButton = msgBox.addButton(
        QObject::tr("Copy Details"), 
        QMessageBox::ActionRole
    );
    msgBox.addButton(QMessageBox::Ok);
    
    msgBox.exec();
    
    // Handle button clicks
    if (msgBox.clickedButton() == openLogsButton) {
        open_log_directory(parent);
    } else if (msgBox.clickedButton() == copyButton) {
        QString copy_text = title + "\n\n" + message;
        if (!details.isEmpty()) {
            copy_text += "\n\nDetails:\n" + details;
        }
        copy_text += "\n\n" + get_log_location_message();
        
        QClipboard* clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(copy_text);
        }
    }
}

void ErrorHandler::log_and_show_error(QWidget* parent,
                                      const std::string& logger_name,
                                      const QString& title,
                                      const QString& user_message,
                                      const std::string& technical_details)
{
    // Log the technical details
    if (auto logger = Logger::get_logger(logger_name)) {
        logger->error("{}", technical_details);
    }
    
    // Show user-friendly message with option to see details
    show_error_with_log_access(
        parent,
        title,
        user_message,
        QString::fromStdString(technical_details)
    );
}

QString ErrorHandler::get_log_location_message()
{
    try {
        std::string log_dir = Logger::get_log_directory();
        return QObject::tr("Log files location:\n%1\n\n"
                          "Check the log files for detailed error information.")
            .arg(QString::fromStdString(log_dir));
    } catch (...) {
        return QObject::tr("Log files location could not be determined.");
    }
}

bool ErrorHandler::open_log_directory(QWidget* parent)
{
    try {
        std::string log_dir = Logger::get_log_directory();
        QUrl url = QUrl::fromLocalFile(QString::fromStdString(log_dir));
        
        if (QDesktopServices::openUrl(url)) {
            return true;
        }
        
        if (parent) {
            QMessageBox::warning(
                parent,
                QObject::tr("Cannot Open"),
                QObject::tr("Failed to open log directory:\n%1\n\n"
                           "Please navigate to this location manually.")
                    .arg(QString::fromStdString(log_dir))
            );
        }
        return false;
    } catch (const std::exception& ex) {
        if (parent) {
            QMessageBox::warning(
                parent,
                QObject::tr("Error"),
                QObject::tr("Could not determine log directory location: %1")
                    .arg(QString::fromStdString(ex.what()))
            );
        }
        return false;
    }
}
