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
