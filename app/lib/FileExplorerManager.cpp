#include "FileExplorerManager.hpp"
#include "Settings.hpp"

#include <QAction>
#include <QDockWidget>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QMainWindow>
#include <QStandardPaths>
#include <QTreeView>

namespace afs {

FileExplorerManager::FileExplorerManager(QMainWindow* parent, Settings& settings)
    : QObject(parent)
    , parent_window_(parent)
    , settings_(settings)
{
}

FileExplorerManager::~FileExplorerManager() = default;

bool FileExplorerManager::setup()
{
    if (!parent_window_) {
        return false;
    }

    create_dock_widget();
    setup_file_system_model();
    setup_tree_view();
    connect_signals();
    apply_preferences();
    
    return true;
}

void FileExplorerManager::create_dock_widget()
{
    explorer_dock_ = new QDockWidget(tr("File Explorer"), parent_window_);
    explorer_dock_->setObjectName("FileExplorerDock");
    explorer_dock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    explorer_dock_->setFeatures(QDockWidget::DockWidgetClosable | 
                                QDockWidget::DockWidgetMovable);

    parent_window_->addDockWidget(Qt::LeftDockWidgetArea, explorer_dock_);

    // Create toggle action for menu
    toggle_action_ = new QAction(tr("Show File Explorer"), parent_window_);
    toggle_action_->setCheckable(true);
    toggle_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
}

void FileExplorerManager::setup_file_system_model()
{
    file_system_model_ = new QFileSystemModel(this);
    file_system_model_->setRootPath(get_default_root_path());
    
    // Only show directories in the explorer
    file_system_model_->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
    
    // Use natural sorting for better UX
    file_system_model_->setOption(QFileSystemModel::DontUseCustomDirectoryIcons, false);
}

void FileExplorerManager::setup_tree_view()
{
    explorer_view_ = new QTreeView(explorer_dock_);
    explorer_view_->setModel(file_system_model_);
    explorer_view_->setRootIndex(file_system_model_->index(get_default_root_path()));

    // Only show the Name column, hide Size, Type, Date Modified
    explorer_view_->setHeaderHidden(true);
    for (int i = 1; i < file_system_model_->columnCount(); ++i) {
        explorer_view_->hideColumn(i);
    }

    // Configure selection behavior
    explorer_view_->setSelectionMode(QAbstractItemView::SingleSelection);
    explorer_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Enable expand-on-double-click
    explorer_view_->setExpandsOnDoubleClick(true);
    
    // Set sensible sizing
    explorer_view_->setMinimumWidth(200);
    explorer_view_->setAnimated(true);
    
    // Context menu support
    explorer_view_->setContextMenuPolicy(Qt::CustomContextMenu);

    explorer_dock_->setWidget(explorer_view_);
}

void FileExplorerManager::connect_signals()
{
    // Tree view selection
    connect(explorer_view_, &QTreeView::clicked,
            this, &FileExplorerManager::on_item_clicked);
    
    // Double-click for entering directories
    connect(explorer_view_, &QTreeView::doubleClicked,
            this, &FileExplorerManager::on_item_clicked);

    // Dock visibility
    connect(explorer_dock_, &QDockWidget::visibilityChanged,
            this, &FileExplorerManager::on_dock_visibility_changed);

    // Toggle action
    connect(toggle_action_, &QAction::toggled, this, [this](bool checked) {
        explorer_dock_->setVisible(checked);
    });
}

void FileExplorerManager::apply_preferences()
{
    // Sync toggle action with dock visibility
    toggle_action_->setChecked(explorer_dock_->isVisible());
}

QString FileExplorerManager::get_default_root_path() const
{
#ifdef Q_OS_WIN
    return QString();  // Show all drives on Windows
#else
    // On Unix, start from home directory
    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return home.isEmpty() ? "/" : home;
#endif
}

void FileExplorerManager::set_visible(bool visible)
{
    if (explorer_dock_) {
        explorer_dock_->setVisible(visible);
    }
    if (toggle_action_) {
        toggle_action_->setChecked(visible);
    }
}

bool FileExplorerManager::is_visible() const
{
    return explorer_dock_ && explorer_dock_->isVisible();
}

void FileExplorerManager::focus_on_path(const QString& path, bool expand)
{
    if (!explorer_view_ || !file_system_model_ || path.isEmpty()) {
        return;
    }

    QModelIndex index = file_system_model_->index(path);
    if (!index.isValid()) {
        return;
    }

    suppress_sync_ = true;

    if (expand) {
        // Expand parent directories
        QModelIndex parent = index.parent();
        while (parent.isValid()) {
            explorer_view_->expand(parent);
            parent = parent.parent();
        }
    }

    explorer_view_->setCurrentIndex(index);
    explorer_view_->scrollTo(index, QAbstractItemView::PositionAtCenter);

    suppress_sync_ = false;
}

void FileExplorerManager::restore_state()
{
    bool show = settings_.get_show_file_explorer();
    set_visible(show);

    // If there's a saved sort folder, focus on it
    QString sort_folder = QString::fromStdString(settings_.get_sort_folder());
    if (!sort_folder.isEmpty()) {
        focus_on_path(sort_folder);
    }
}

void FileExplorerManager::save_state()
{
    settings_.set_show_file_explorer(is_visible());
}

void FileExplorerManager::set_directory_selected_callback(DirectorySelectedCallback callback)
{
    directory_callback_ = std::move(callback);
}

void FileExplorerManager::on_item_clicked(const QModelIndex& index)
{
    if (!index.isValid() || suppress_sync_) {
        return;
    }

    QString path = file_system_model_->filePath(index);
    if (path.isEmpty()) {
        return;
    }

    // Only process directories
    if (!file_system_model_->isDir(index)) {
        return;
    }

    emit directory_selected(path);

    if (directory_callback_) {
        directory_callback_(path, true /* user_initiated */);
    }
}

void FileExplorerManager::on_dock_visibility_changed(bool visible)
{
    if (toggle_action_) {
        toggle_action_->setChecked(visible);
    }
    emit visibility_changed(visible);
}

} // namespace afs
