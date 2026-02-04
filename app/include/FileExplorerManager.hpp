#ifndef FILE_EXPLORER_MANAGER_HPP
#define FILE_EXPLORER_MANAGER_HPP

#include <QObject>
#include <QPointer>
#include <QString>

#include <functional>
#include <memory>

class QDockWidget;
class QTreeView;
class QFileSystemModel;
class QAction;
class QMainWindow;
class Settings;

namespace afs {

/**
 * @brief Manages the file explorer sidebar functionality.
 * 
 * Encapsulates all file explorer-related operations that were previously
 * scattered throughout MainApp. Handles:
 * - Tree view setup and configuration
 * - Directory navigation
 * - User preferences (visibility, expanded state)
 * - Signal connections
 */
class FileExplorerManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Callback signature for directory selection.
     */
    using DirectorySelectedCallback = std::function<void(const QString& path, bool user_initiated)>;

    /**
     * @brief Constructs the file explorer manager.
     * @param parent The main window that will host the dock widget
     * @param settings Application settings for persistence
     */
    explicit FileExplorerManager(QMainWindow* parent, Settings& settings);
    ~FileExplorerManager() override;

    /**
     * @brief Creates and sets up all file explorer UI components.
     * @return true if setup succeeded
     */
    bool setup();

    /**
     * @brief Gets the dock widget containing the file explorer.
     */
    [[nodiscard]] QDockWidget* dock_widget() const { return explorer_dock_; }

    /**
     * @brief Gets the tree view component.
     */
    [[nodiscard]] QTreeView* tree_view() const { return explorer_view_; }

    /**
     * @brief Gets the menu action for toggling visibility.
     */
    [[nodiscard]] QAction* toggle_action() const { return toggle_action_; }

    /**
     * @brief Sets whether the explorer is visible.
     */
    void set_visible(bool visible);

    /**
     * @brief Checks if the explorer is visible.
     */
    [[nodiscard]] bool is_visible() const;

    /**
     * @brief Navigates to and selects the specified path.
     * @param path The directory path to navigate to
     * @param expand Whether to expand the tree to show the path
     */
    void focus_on_path(const QString& path, bool expand = true);

    /**
     * @brief Restores the explorer state from settings.
     */
    void restore_state();

    /**
     * @brief Saves the current explorer state to settings.
     */
    void save_state();

    /**
     * @brief Sets the callback for directory selection events.
     */
    void set_directory_selected_callback(DirectorySelectedCallback callback);

    /**
     * @brief Suppresses directory selection callbacks temporarily.
     * @param suppress Whether to suppress callbacks
     */
    void set_suppress_sync(bool suppress) { suppress_sync_ = suppress; }

signals:
    /**
     * @brief Emitted when the visibility state changes.
     */
    void visibility_changed(bool visible);

    /**
     * @brief Emitted when a directory is selected by the user.
     */
    void directory_selected(const QString& path);

private slots:
    void on_item_clicked(const QModelIndex& index);
    void on_dock_visibility_changed(bool visible);

private:
    void create_dock_widget();
    void setup_file_system_model();
    void setup_tree_view();
    void connect_signals();
    void apply_preferences();
    QString get_default_root_path() const;

    QMainWindow* parent_window_;
    Settings& settings_;
    
    QPointer<QDockWidget> explorer_dock_;
    QPointer<QTreeView> explorer_view_;
    QPointer<QFileSystemModel> file_system_model_;
    QPointer<QAction> toggle_action_;

    DirectorySelectedCallback directory_callback_;
    bool suppress_sync_{false};
};

} // namespace afs

#endif // FILE_EXPLORER_MANAGER_HPP
