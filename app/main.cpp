#include "EmbeddedEnv.hpp"
#include "Logger.hpp"
#include "MainApp.hpp"
#include "Utils.hpp"
#include "LLMSelectionDialog.hpp"
#include "StartupErrorDialog.hpp"
#include "Settings.hpp"
#include <app_version.hpp>

#include <QApplication>
#include <QDialog>
#include <QGuiApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QSize>
#include <QElapsedTimer>
#include <QTimer>

#include <functional>
#include <algorithm>
#include <vector>
#include <cstring>
#include <QPainter>
#include <memory>
#include <fstream>
#include <filesystem>

#include <curl/curl.h>
#include <locale.h>
#include <libintl.h>
#include <cstdio>
#include <iostream>
#include <fmt/format.h>
#ifdef _WIN32
#include <windows.h>
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif
using SetProcessDpiAwarenessContextFn = BOOL (WINAPI *)(HANDLE);
using SetProcessDpiAwarenessFn = HRESULT (WINAPI *)(int); // 2 = PROCESS_PER_MONITOR_DPI_AWARE
#endif


bool initialize_loggers()
{
    try {
        Logger::setup_loggers();
        return true;
    } catch (const std::exception &e) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->critical("Failed to initialize loggers: {}", e.what());
        } else {
            std::fprintf(stderr, "Failed to initialize loggers: %s\n", e.what());
        }
        return false;
    }
}

namespace {

// Helper to get core logger safely
inline std::shared_ptr<spdlog::logger> get_core_logger() {
    return Logger::get_logger("core_logger");
}

// Common error message constants
constexpr const char* ERR_MAINAPP_INIT_FAILED = "Failed to initialize main application window";

struct ParsedArguments {
    bool development_mode{false};
    bool console_log{false};
    bool force_direct_run{false};
    std::vector<char*> qt_args;
};

ParsedArguments parse_command_line(int argc, char** argv)
{
    ParsedArguments parsed;
    parsed.qt_args.reserve(static_cast<size_t>(argc) + 1);

    for (int i = 0; i < argc; ++i) {
        const bool is_flag = (i > 0);
        if (is_flag && std::strcmp(argv[i], "--development") == 0) {
            parsed.development_mode = true;
            continue;
        }
        if (is_flag && std::strcmp(argv[i], "--allow-direct-launch") == 0) {
            continue;
        }
        if (is_flag && std::strcmp(argv[i], "--console-log") == 0) {
            parsed.console_log = true;
            continue;
        }
        if (is_flag && std::strcmp(argv[i], "--force-direct-run") == 0) {
            parsed.force_direct_run = true;
            continue;
        }
        parsed.qt_args.push_back(argv[i]);
    }
    parsed.qt_args.push_back(nullptr);
    return parsed;
}

#ifdef _WIN32
bool allow_direct_launch(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--force-direct-run") == 0) {
            return true;
        }
    }
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--allow-direct-launch") == 0) {
            return true;
        }
    }
    return false;
}

void enable_per_monitor_dpi_awareness()
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        const auto set_ctx = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (set_ctx && set_ctx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            return;
        }
    }
    HMODULE shcore = LoadLibraryW(L"Shcore.dll");
    if (shcore) {
        const auto set_awareness = reinterpret_cast<SetProcessDpiAwarenessFn>(
            GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (set_awareness) {
            // 2 == PROCESS_PER_MONITOR_DPI_AWARE
            set_awareness(2);
        }
        FreeLibrary(shcore);
    }
}

void attach_console_if_requested(bool enable)
{
    if (!enable) {
        return;
    }
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* f = nullptr;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
        freopen_s(&f, "CONIN$", "r", stdin);
    }
}
#endif

[[maybe_unused]] QPixmap build_splash_pixmap()
{
    QPixmap splash_pix(QStringLiteral(":/net/quicknode/AIFileSorter/images/icon_512x512.png"));
    if (splash_pix.isNull()) {
        splash_pix = QPixmap(256, 256);
        splash_pix.fill(Qt::black);
    }

    const QSize base_size(320, 320);
    const QSize padded_size(static_cast<int>(base_size.width() * 1.2),
                            static_cast<int>(base_size.height() * 1.1));

    QPixmap scaled_splash = splash_pix.scaled(base_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap splash_canvas(padded_size);
    splash_canvas.fill(QColor(QStringLiteral("#f5e6d3")));

    QPainter painter(&splash_canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QPoint centered_icon((padded_size.width() - scaled_splash.width()) / 2,
                               (padded_size.height() - scaled_splash.height()) / 2 - 10);
    painter.drawPixmap(centered_icon, scaled_splash);
    painter.end();

    return splash_canvas;
}

class SplashController {
public:
    explicit SplashController(QApplication& app)
        : app_(app)
    {
    }

    void set_target(QWidget* target)
    {
        target_ = target;
    }

    void keep_visible_for(int minimum_duration_ms)
    {
        Q_UNUSED(minimum_duration_ms);
    }

    void finish()
    {
        finished_ = true;
    }

private:
    QApplication& app_;
    bool finished_{false};
    QWidget* target_{nullptr};
};

bool ensure_llm_choice(Settings& settings, const std::function<void()>& finish_splash)
{
    if (settings.get_llm_choice() != LLMChoice::Unset) {
        return true;
    }

    LLMSelectionDialog llm_dialog(settings);
    if (llm_dialog.exec() != QDialog::Accepted) {
        if (finish_splash) {
            finish_splash();
        }
        return false;
    }

    settings.set_llm_choice(llm_dialog.get_selected_llm_choice());
    settings.save();
    return true;
}

struct PreFlightCheck {
    bool success;
    std::string error_message;
    std::string details;
};

PreFlightCheck validate_startup_environment()
{
    PreFlightCheck result{true, "", ""};
    
    // Check if we can access the config directory
    try {
        Settings test_settings;
        std::string config_dir = test_settings.get_config_dir();
        
        // Try to create directory if it doesn't exist
        std::filesystem::path config_path(config_dir);
        if (!std::filesystem::exists(config_path)) {
            try {
                std::filesystem::create_directories(config_path);
            } catch (const std::exception& ex) {
                result.success = false;
                result.error_message = "Cannot create application configuration directory";
                result.details = fmt::format("Path: {}\nError: {}", config_dir, ex.what());
                return result;
            }
        }
        
        // Check if directory is writable by trying to create a test file
        std::filesystem::path test_file = config_path / ".write_test";
        try {
            std::ofstream test_stream(test_file);
            if (!test_stream) {
                result.success = false;
                result.error_message = "Configuration directory is not writable";
                result.details = fmt::format("Path: {}\nPlease check file permissions", config_dir);
                return result;
            }
            test_stream.close();
            std::filesystem::remove(test_file);
        } catch (const std::exception& ex) {
            result.success = false;
            result.error_message = "Cannot write to configuration directory";
            result.details = fmt::format("Path: {}\nError: {}", config_dir, ex.what());
            return result;
        }
        
    } catch (const std::exception& ex) {
        result.success = false;
        result.error_message = "Failed to validate configuration directory";
        result.details = ex.what();
        return result;
    }
    
    // Check log directory access
    try {
        std::string log_dir = Logger::get_log_directory();
        std::filesystem::path log_path(log_dir);
        
        if (!std::filesystem::exists(log_path)) {
            try {
                std::filesystem::create_directories(log_path);
            } catch (const std::exception& ex) {
                // Log directory creation failure is not fatal, but worth noting
                if (auto logger = get_core_logger()) {
                    logger->warn("Could not create log directory: {}", ex.what());
                }
            }
        }
    } catch (const std::exception& ex) {
        // Log directory issues are not fatal
        if (auto logger = get_core_logger()) {
            logger->warn("Log directory validation issue: {}", ex.what());
        }
    }
    
    return result;
}

int run_application(const ParsedArguments& parsed_args)
{
    // Initialize QApplication early so we can show error dialogs
    int qt_argc = static_cast<int>(parsed_args.qt_args.size()) - 1;
    char** qt_argv = const_cast<char**>(parsed_args.qt_args.data());
    QApplication app(qt_argc, qt_argv);

    QCoreApplication::setApplicationName(QStringLiteral("AI File Sorter"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("AI File Sorter"));

    try {
        // Run pre-flight checks before attempting to start the application
        PreFlightCheck preflight = validate_startup_environment();
        if (!preflight.success) {
            if (auto logger = get_core_logger()) {
                logger->critical("Pre-flight check failed: {}", preflight.details);
            }
            StartupErrorDialog::show_startup_error(preflight.error_message, preflight.details);
            return EXIT_FAILURE;
        }

        // Load environment and locale settings
        EmbeddedEnv env_loader(":/net/quicknode/AIFileSorter/.env");
        env_loader.load_env();
        
        setlocale(LC_ALL, "");
        const std::string locale_path = Utils::get_executable_path() + "/locale";
        bindtextdomain("net.quicknode.AIFileSorter", locale_path.c_str());

        // Load settings
        Settings settings;
        try {
            settings.load();
        } catch (const std::exception& ex) {
            std::string error_msg = "Failed to load application settings";
            if (auto logger = get_core_logger()) {
                logger->critical("Settings load failed: {}", ex.what());
            }
            StartupErrorDialog::show_startup_error(error_msg, ex.what());
            return EXIT_FAILURE;
        }

        const auto finish_splash = [&]() {};

        // Ensure LLM is configured
        if (!ensure_llm_choice(settings, finish_splash)) {
            return EXIT_SUCCESS;
        }

        // Initialize main application with error handling
        std::unique_ptr<MainApp> main_app;
        try {
            main_app = std::make_unique<MainApp>(settings, parsed_args.development_mode);
        } catch (const std::exception& ex) {
            if (auto logger = get_core_logger()) {
                logger->critical("MainApp initialization failed: {}", ex.what());
            }
            StartupErrorDialog::show_startup_error(ERR_MAINAPP_INIT_FAILED, ex.what());
            return EXIT_FAILURE;
        } catch (...) {
            std::string details = "Unknown error occurred during initialization";
            if (auto logger = get_core_logger()) {
                logger->critical("MainApp initialization failed: unknown error");
            }
            StartupErrorDialog::show_startup_error(ERR_MAINAPP_INIT_FAILED, details);
            return EXIT_FAILURE;
        }

        // Run the application
        main_app->run();

        const int result = app.exec();
        main_app->shutdown();
        return result;

    } catch (const std::exception& ex) {
        std::string error_msg = "Critical startup error";
        if (auto logger = get_core_logger()) {
            logger->critical("Critical startup error: {}", ex.what());
        }
        StartupErrorDialog::show_startup_error(error_msg, ex.what());
        return EXIT_FAILURE;
    } catch (...) {
        std::string error_msg = "Critical startup error";
        std::string details = "Unknown exception during application startup";
        if (auto logger = get_core_logger()) {
            logger->critical("Critical startup error: unknown exception");
        }
        StartupErrorDialog::show_startup_error(error_msg, details);
        return EXIT_FAILURE;
    }
}

} // namespace


int main(int argc, char **argv) {

    ParsedArguments parsed = parse_command_line(argc, argv);

#ifdef _WIN32
    enable_per_monitor_dpi_awareness();
    attach_console_if_requested(parsed.console_log);
#endif

    // Initialize loggers first - this is critical for error reporting
    if (!initialize_loggers()) {
        // Logger initialization failed - show basic error dialog
        std::fprintf(stderr, "FATAL: Failed to initialize logging system\n");
        
        // Try to show a basic message box if possible
        try {
            int dummy_argc = 1;
            char* dummy_argv[] = {argv[0], nullptr};
            QApplication temp_app(dummy_argc, dummy_argv);
            StartupErrorDialog::show_startup_error(
                "Logger Initialization Failed",
                "The application could not initialize its logging system. "
                "This may indicate a permissions issue or missing directories."
            );
        } catch (...) {
            // Even QApplication failed, just exit
        }
        return EXIT_FAILURE;
    }

    // Initialize CURL for network operations
    curl_global_init(CURL_GLOBAL_DEFAULT);
    struct CurlCleanup {
        ~CurlCleanup() { curl_global_cleanup(); }
    } curl_cleanup;

    #ifdef _WIN32
        _putenv("GSETTINGS_SCHEMA_DIR=schemas");
    #endif

    // Run the application with comprehensive error handling
    try {
        return run_application(parsed);
    } catch (const std::exception& ex) {
        // Log the critical error
        if (auto logger = get_core_logger()) {
            logger->critical("Unhandled exception in main: {}", ex.what());
        } else {
            std::fprintf(stderr, "FATAL ERROR: %s\n", ex.what());
        }
        
        // Try to show error dialog
        try {
            StartupErrorDialog::show_startup_error(
                "Fatal Application Error",
                std::string("Unhandled exception: ") + ex.what()
            );
        } catch (...) {
            std::fprintf(stderr, "Failed to show error dialog\n");
        }
        
        return EXIT_FAILURE;
    } catch (...) {
        // Unknown exception
        if (auto logger = get_core_logger()) {
            logger->critical("Unhandled unknown exception in main");
        } else {
            std::fprintf(stderr, "FATAL ERROR: Unknown exception\n");
        }
        
        // Try to show error dialog
        try {
            StartupErrorDialog::show_startup_error(
                "Fatal Application Error",
                "An unknown error occurred during application startup"
            );
        } catch (...) {
            std::fprintf(stderr, "Failed to show error dialog\n");
        }
        
        return EXIT_FAILURE;
    }
}
