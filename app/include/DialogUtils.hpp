#ifndef DIALOGUTILS_HPP
#define DIALOGUTILS_HPP

#include <string>

class QWidget;

namespace ErrorCodes {
    class AppException;
    enum class Code;
}

class DialogUtils {
public:
    // Show simple error dialog with message (includes copy button)
    static void show_error_dialog(QWidget* parent, const std::string& message);
    
    // Show error dialog with error code and full details (includes copy button)
    static void show_error_dialog(QWidget* parent, ErrorCodes::Code error_code, const std::string& context = "");
    
    // Show error dialog from AppException (includes copy button)
    static void show_error_dialog(QWidget* parent, const ErrorCodes::AppException& exception);
    
    // Show warning dialog with copyable text
    static void show_warning_dialog(QWidget* parent, const std::string& title, const std::string& message);
    
    // Show information dialog with copyable text
    static void show_info_dialog(QWidget* parent, const std::string& title, const std::string& message);
    
    // Show startup error dialog with error report file location and copy functionality
    // This is specifically designed to help users provide error context when reporting issues
    static void show_startup_error_dialog(QWidget* parent, 
                                         const std::string& error_message,
                                         const std::string& error_report_file = "");
};

#endif // DIALOGUTILS_HPP
