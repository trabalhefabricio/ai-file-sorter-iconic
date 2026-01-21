#include "DialogUtils.hpp"
#include "AppException.hpp"
#include "ErrorCode.hpp"

#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QPushButton>
#include <QClipboard>
#include <QApplication>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>

void DialogUtils::show_error_dialog(QWidget* parent, const std::string& message)
{
    QMessageBox msg_box(parent);
    msg_box.setIcon(QMessageBox::Critical);
    msg_box.setWindowTitle(QObject::tr("Error"));
    msg_box.setText(QString::fromStdString(message));
    
    // Make text selectable for copying
    msg_box.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    
    // Add "Copy to Clipboard" button
    QPushButton* copy_button = msg_box.addButton(QObject::tr("Copy to Clipboard"), QMessageBox::ActionRole);
    msg_box.addButton(QMessageBox::Ok);
    
    msg_box.exec();
    
    // If user clicked "Copy to Clipboard", copy message to clipboard
    if (msg_box.clickedButton() == copy_button) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(QString::fromStdString(message));
        
        // Show confirmation
        QMessageBox::information(parent, QObject::tr("Copied"),
            QObject::tr("Error message has been copied to clipboard."));
    }
}

void DialogUtils::show_error_dialog(QWidget* parent, ErrorCodes::Code error_code, const std::string& context)
{
    auto error_info = ErrorCodes::ErrorCatalog::get_error_info(error_code, context);
    
    QMessageBox msg_box(parent);
    msg_box.setIcon(QMessageBox::Critical);
    msg_box.setWindowTitle(QObject::tr("Error"));
    msg_box.setText(QString::fromStdString(error_info.get_user_message()));
    
    // Make text selectable for copying
    msg_box.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    
    // Add "Copy Error Details" button
    QPushButton* copy_button = msg_box.addButton(QObject::tr("Copy Error Details"), QMessageBox::ActionRole);
    msg_box.addButton(QMessageBox::Ok);
    
    msg_box.exec();
    
    // If user clicked "Copy Error Details", copy full details to clipboard
    if (msg_box.clickedButton() == copy_button) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(QString::fromStdString(error_info.get_full_details()));
        
        // Show confirmation
        QMessageBox::information(parent, QObject::tr("Copied"),
            QObject::tr("Error details have been copied to clipboard."));
    }
}

void DialogUtils::show_error_dialog(QWidget* parent, const ErrorCodes::AppException& exception)
{
    const auto& error_info = exception.get_error_info();
    
    QMessageBox msg_box(parent);
    msg_box.setIcon(QMessageBox::Critical);
    msg_box.setWindowTitle(QObject::tr("Error"));
    msg_box.setText(QString::fromStdString(error_info.get_user_message()));
    
    // Make text selectable for copying
    msg_box.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    
    // Add "Copy Error Details" button
    QPushButton* copy_button = msg_box.addButton(QObject::tr("Copy Error Details"), QMessageBox::ActionRole);
    msg_box.addButton(QMessageBox::Ok);
    
    msg_box.exec();
    
    // If user clicked "Copy Error Details", copy full details to clipboard
    if (msg_box.clickedButton() == copy_button) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(QString::fromStdString(error_info.get_full_details()));
        
        // Show confirmation
        QMessageBox::information(parent, QObject::tr("Copied"),
            QObject::tr("Error details have been copied to clipboard."));
    }
}

void DialogUtils::show_warning_dialog(QWidget* parent, const std::string& title, const std::string& message)
{
    QMessageBox msg_box(parent);
    msg_box.setIcon(QMessageBox::Warning);
    msg_box.setWindowTitle(QString::fromStdString(title));
    msg_box.setText(QString::fromStdString(message));
    
    // Make text selectable for copying
    msg_box.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    
    // Add "Copy to Clipboard" button
    QPushButton* copy_button = msg_box.addButton(QObject::tr("Copy to Clipboard"), QMessageBox::ActionRole);
    msg_box.addButton(QMessageBox::Ok);
    
    msg_box.exec();
    
    // If user clicked "Copy to Clipboard", copy message to clipboard
    if (msg_box.clickedButton() == copy_button) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(QString::fromStdString(message));
        
        // Show confirmation (brief, don't interrupt workflow)
        QMessageBox confirmation(parent);
        confirmation.setIcon(QMessageBox::Information);
        confirmation.setWindowTitle(QObject::tr("Copied"));
        confirmation.setText(QObject::tr("Message copied to clipboard."));
        confirmation.setStandardButtons(QMessageBox::Ok);
        confirmation.setDefaultButton(QMessageBox::Ok);
        QTimer::singleShot(1500, &confirmation, &QDialog::accept);
        confirmation.exec();
    }
}

void DialogUtils::show_info_dialog(QWidget* parent, const std::string& title, const std::string& message)
{
    QMessageBox msg_box(parent);
    msg_box.setIcon(QMessageBox::Information);
    msg_box.setWindowTitle(QString::fromStdString(title));
    msg_box.setText(QString::fromStdString(message));
    
    // Make text selectable for copying
    msg_box.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    
    // Add "Copy to Clipboard" button
    QPushButton* copy_button = msg_box.addButton(QObject::tr("Copy to Clipboard"), QMessageBox::ActionRole);
    msg_box.addButton(QMessageBox::Ok);
    
    msg_box.exec();
    
    // If user clicked "Copy to Clipboard", copy message to clipboard
    if (msg_box.clickedButton() == copy_button) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(QString::fromStdString(message));
    }
}

void DialogUtils::show_startup_error_dialog(QWidget* parent, 
                                           const std::string& error_message,
                                           const std::string& error_report_file)
{
    QMessageBox msg_box(parent);
    msg_box.setIcon(QMessageBox::Critical);
    msg_box.setWindowTitle(QObject::tr("Startup Error"));
    
    // Build the message
    QString full_message = QString::fromStdString(error_message);
    
    // Add information about error report file if provided
    if (!error_report_file.empty()) {
        QFileInfo file_info(QString::fromStdString(error_report_file));
        if (file_info.exists()) {
            full_message.append(
                QString("\n\n%1\n%2\n\n%3\n%4\n%5\n%6")
                .arg(QObject::tr("📋 Detailed Error Report:"))
                .arg(file_info.absoluteFilePath())
                .arg(QObject::tr("💡 To get help:"))
                .arg(QObject::tr("1. Click 'Copy Error Report' below"))
                .arg(QObject::tr("2. Paste into GitHub Copilot Chat or GitHub issue"))
                .arg(QObject::tr("3. Ask: \"How do I fix this error?\""))
            );
        } else {
            // File doesn't exist yet, just tell them where logs are
            QDir log_dir = file_info.dir();
            full_message.append(
                QString("\n\n%1\n%2\n\n%3")
                .arg(QObject::tr("📋 Logs directory:"))
                .arg(log_dir.absolutePath())
                .arg(QObject::tr("💡 Check the logs directory for detailed error information."))
            );
        }
    } else {
        full_message.append(
            QString("\n\n%1\n%2\n%3\n%4")
            .arg(QObject::tr("💡 To report this issue:"))
            .arg(QObject::tr("1. Click 'Copy Error Info' below"))
            .arg(QObject::tr("2. Paste into GitHub Copilot Chat or create a GitHub issue"))
            .arg(QObject::tr("3. Include any additional context about what you were doing"))
        );
    }
    
    msg_box.setText(full_message);
    
    // Make text selectable for copying
    msg_box.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    
    // Add buttons
    QPushButton* copy_report_button = nullptr;
    QPushButton* open_logs_button = nullptr;
    
    if (!error_report_file.empty()) {
        QFileInfo file_info(QString::fromStdString(error_report_file));
        if (file_info.exists()) {
            // Add "Copy Error Report" button to copy the entire error report file
            copy_report_button = msg_box.addButton(QObject::tr("Copy Error Report"), QMessageBox::ActionRole);
            
            // Add "Open Logs Folder" button
            open_logs_button = msg_box.addButton(QObject::tr("Open Logs Folder"), QMessageBox::ActionRole);
        }
    }
    
    // Add "Copy Error Info" button to copy just the error message
    QPushButton* copy_message_button = msg_box.addButton(QObject::tr("Copy Error Info"), QMessageBox::ActionRole);
    
    // Add standard OK button
    msg_box.addButton(QMessageBox::Ok);
    
    msg_box.exec();
    
    QAbstractButton* clicked = msg_box.clickedButton();
    
    // Handle button clicks
    if (clicked == copy_report_button && copy_report_button) {
        // Read and copy the entire error report file
        QFile file(QString::fromStdString(error_report_file));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString file_content = in.readAll();
            file.close();
            
            QClipboard* clipboard = QApplication::clipboard();
            clipboard->setText(file_content);
            
            // Show confirmation
            QMessageBox::information(parent, QObject::tr("Copied"),
                QObject::tr("Error report has been copied to clipboard.\n\n"
                           "Paste it into GitHub Copilot Chat or a GitHub issue for help."),
                QMessageBox::Ok);
        } else {
            QMessageBox::warning(parent, QObject::tr("Error"),
                QObject::tr("Could not read error report file."));
        }
    } else if (clicked == open_logs_button && open_logs_button) {
        // Open the logs folder in file explorer
        QFileInfo file_info(QString::fromStdString(error_report_file));
        QDesktopServices::openUrl(QUrl::fromLocalFile(file_info.dir().absolutePath()));
    } else if (clicked == copy_message_button) {
        // Copy just the error message
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(full_message);
        
        // Show brief confirmation
        QMessageBox::information(parent, QObject::tr("Copied"),
            QObject::tr("Error information has been copied to clipboard."));
    }
}
