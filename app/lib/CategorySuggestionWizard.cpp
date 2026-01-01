#include "CategorySuggestionWizard.hpp"

#include <QFileInfo>
#include <QPixmap>
#include <QImageReader>
#include <QGroupBox>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>
#include <sstream>
#include <algorithm>

CategorySuggestionWizard::CategorySuggestionWizard(
    const FileEntry& file,
    const std::string& suggested_parent,
    double confidence_score,
    const std::vector<std::string>& existing_paths,
    QWidget* parent)
    : QDialog(parent)
    , file_(file)
    , parent_path_(suggested_parent)
    , confidence_score_(confidence_score)
    , existing_paths_(existing_paths)
    , result_(Skip)
    , selected_path_("")
{
    setWindowTitle("Create New Category?");
    setModal(true);
    setMinimumWidth(500);
    
    setup_ui();
    load_file_preview();
}

void CategorySuggestionWizard::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(15);
    main_layout->setContentsMargins(20, 20, 20, 20);

    create_file_preview_section();
    create_options_section();
    create_buttons_section();

    // Default selection
    use_parent_radio_->setChecked(true);
    on_radio_changed();
}

void CategorySuggestionWizard::create_file_preview_section() {
    auto* preview_group = new QGroupBox("File Information", this);
    auto* preview_layout = new QVBoxLayout(preview_group);

    // File preview (image thumbnail or file icon)
    file_preview_label_ = new QLabel(this);
    file_preview_label_->setAlignment(Qt::AlignCenter);
    file_preview_label_->setFixedHeight(150);
    file_preview_label_->setStyleSheet("QLabel { border: 1px solid #ccc; background-color: #f5f5f5; }");
    preview_layout->addWidget(file_preview_label_);

    // File info text
    file_info_label_ = new QLabel(this);
    file_info_label_->setWordWrap(true);
    preview_layout->addWidget(file_info_label_);

    // AI suggestion
    ai_suggestion_label_ = new QLabel(this);
    ai_suggestion_label_->setWordWrap(true);
    ai_suggestion_label_->setStyleSheet("QLabel { color: #0066cc; font-weight: bold; }");
    preview_layout->addWidget(ai_suggestion_label_);

    layout()->addWidget(preview_group);
}

void CategorySuggestionWizard::create_options_section() {
    auto* options_group = new QGroupBox("What would you like to do?", this);
    auto* options_layout = new QVBoxLayout(options_group);
    options_layout->setSpacing(10);

    option_group_ = new QButtonGroup(this);

    // Option 1: Use parent
    use_parent_radio_ = new QRadioButton(this);
    if (parent_path_.empty()) {
        use_parent_radio_->setText("Place in root folder");
        use_parent_radio_->setEnabled(false);
    } else {
        use_parent_radio_->setText(QString("Use parent category: %1")
            .arg(QString::fromStdString(parent_path_)));
    }
    option_group_->addButton(use_parent_radio_, 0);
    options_layout->addWidget(use_parent_radio_);

    // Option 2: Create new
    create_new_radio_ = new QRadioButton("Create new subcategory:", this);
    option_group_->addButton(create_new_radio_, 1);
    options_layout->addWidget(create_new_radio_);

    // Subcategory input field
    auto* input_layout = new QHBoxLayout();
    input_layout->setContentsMargins(30, 0, 0, 0);
    
    subcategory_label_ = new QLabel(this);
    if (!parent_path_.empty()) {
        subcategory_label_->setText(QString::fromStdString(parent_path_) + " / ");
    }
    input_layout->addWidget(subcategory_label_);

    subcategory_input_ = new QLineEdit(this);
    subcategory_input_->setPlaceholderText("Enter subcategory name");
    connect(subcategory_input_, &QLineEdit::textChanged,
            this, &CategorySuggestionWizard::on_subcategory_input_changed);
    input_layout->addWidget(subcategory_input_, 1);
    
    options_layout->addLayout(input_layout);

    // Validation label
    validation_label_ = new QLabel(this);
    validation_label_->setStyleSheet("QLabel { color: #cc0000; margin-left: 30px; }");
    validation_label_->setWordWrap(true);
    options_layout->addWidget(validation_label_);

    // Option 3: Skip
    skip_radio_ = new QRadioButton("Skip this file", this);
    option_group_->addButton(skip_radio_, 2);
    options_layout->addWidget(skip_radio_);

    // Apply to similar checkbox
    QFileInfo file_info_temp(QString::fromStdString(file_.file_name));
    apply_to_similar_checkbox_ = new QCheckBox(
        QString("Apply to similar files (%1) in this batch")
            .arg(file_info_temp.suffix()), 
        this);
    apply_to_similar_checkbox_->setToolTip(
        "Automatically use this category for other files with the same extension");
    options_layout->addWidget(apply_to_similar_checkbox_);

    connect(option_group_, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            [this](QAbstractButton*) { on_radio_changed(); });

    layout()->addWidget(options_group);
}

void CategorySuggestionWizard::create_buttons_section() {
    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();

    create_button_ = new QPushButton("Create && Continue", this);
    create_button_->setDefault(true);
    connect(create_button_, &QPushButton::clicked,
            this, &CategorySuggestionWizard::on_create_clicked);
    button_layout->addWidget(create_button_);

    cancel_button_ = new QPushButton("Cancel", this);
    connect(cancel_button_, &QPushButton::clicked,
            this, &CategorySuggestionWizard::on_cancel_clicked);
    button_layout->addWidget(cancel_button_);

    // Cast layout to QVBoxLayout to add the button layout
    if (auto* main_layout = qobject_cast<QVBoxLayout*>(layout())) {
        main_layout->addLayout(button_layout);
    }
}

void CategorySuggestionWizard::load_file_preview() {
    QFileInfo file_info(QString::fromStdString(file_.full_path));

    // Try to load image preview
    bool is_image = false;
    QStringList image_extensions = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".tiff"};
    QString ext = file_info.suffix().toLower();
    if (!ext.isEmpty()) {
        ext = "." + ext;  // Add dot prefix for comparison
    }
    
    if (image_extensions.contains(ext)) {
        QImageReader reader(QString::fromStdString(file_.full_path));
        if (reader.canRead()) {
            QPixmap pixmap = QPixmap::fromImage(reader.read());
            if (!pixmap.isNull()) {
                // Scale to fit preview area while maintaining aspect ratio
                pixmap = pixmap.scaled(200, 130, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                file_preview_label_->setPixmap(pixmap);
                is_image = true;
            }
        }
    }

    // Fallback: show file icon and type
    if (!is_image) {
        QString file_type = ext.isEmpty() ? "File" : ext.mid(1).toUpper() + " File";
        file_preview_label_->setText(QString("[File]\n%1").arg(file_type));
        file_preview_label_->setStyleSheet(
            "QLabel { border: 1px solid #ccc; background-color: #f5f5f5; "
            "font-size: 14pt; color: #666; }");
    }

    // File information
    QString size_str = format_file_size(file_info.size());
    QString modified = file_info.lastModified().toString("MMM dd, yyyy");
    
    file_info_label_->setText(
        QString("<b>File:</b> %1<br>"
                "<b>Size:</b> %2 &nbsp;&nbsp; <b>Modified:</b> %3")
            .arg(QString::fromStdString(file_.file_name))
            .arg(size_str)
            .arg(modified));

    // AI suggestion
    QString confidence_str = QString::number(confidence_score_, 'f', 2);
    if (parent_path_.empty()) {
        ai_suggestion_label_->setText(
            QString("Warning: AI is uncertain about this file (confidence: %1)")
                .arg(confidence_str));
    } else {
        ai_suggestion_label_->setText(
            QString("AI Suggestion: %1 (confidence: %2)")
                .arg(QString::fromStdString(parent_path_))
                .arg(confidence_str));
    }
}

void CategorySuggestionWizard::on_radio_changed() {
    bool create_mode = create_new_radio_->isChecked();
    
    subcategory_label_->setEnabled(create_mode);
    subcategory_input_->setEnabled(create_mode);
    validation_label_->setVisible(false);

    if (create_mode) {
        subcategory_input_->setFocus();
        on_subcategory_input_changed(subcategory_input_->text());
    }
}

void CategorySuggestionWizard::on_subcategory_input_changed(const QString& text) {
    if (!create_new_radio_->isChecked()) {
        return;
    }

    QString error = validate_input();
    
    if (error.isEmpty()) {
        validation_label_->setVisible(false);
        create_button_->setEnabled(true);
    } else {
        validation_label_->setText("Warning: " + error);
        validation_label_->setVisible(true);
        create_button_->setEnabled(false);
    }
}

void CategorySuggestionWizard::on_create_clicked() {
    if (skip_radio_->isChecked()) {
        result_ = Skip;
        selected_path_ = "";
        accept();
        return;
    }

    if (use_parent_radio_->isChecked()) {
        result_ = UseParent;
        selected_path_ = parent_path_;
        accept();
        return;
    }

    if (create_new_radio_->isChecked()) {
        QString error = validate_input();
        if (!error.isEmpty()) {
            validation_label_->setText("Warning: " + error);
            validation_label_->setVisible(true);
            return;
        }

        result_ = CreateNew;
        
        std::string subcategory = subcategory_input_->text().trimmed().toStdString();
        if (parent_path_.empty()) {
            selected_path_ = subcategory;
        } else {
            selected_path_ = parent_path_ + "/" + subcategory;
        }
        
        accept();
        return;
    }
}

void CategorySuggestionWizard::on_cancel_clicked() {
    result_ = Skip;
    selected_path_ = "";
    reject();
}

QString CategorySuggestionWizard::validate_input() {
    QString input = subcategory_input_->text().trimmed();

    if (input.isEmpty()) {
        return "Category name cannot be empty";
    }

    std::string segment = input.toStdString();
    
    if (!is_valid_path_segment(segment)) {
        return "Category name contains invalid characters (/, \\, :, *, ?, \", <, >, |)";
    }

    std::string full_path = parent_path_.empty() ? segment : (parent_path_ + "/" + segment);
    
    if (full_path.length() > 255) {
        return "Path is too long (maximum 255 characters)";
    }

    if (count_path_depth(full_path) > 10) {
        return "Maximum nesting depth (10 levels) exceeded";
    }

    // Check for duplicate
    if (std::find(existing_paths_.begin(), existing_paths_.end(), full_path) != existing_paths_.end()) {
        return "This category already exists";
    }

    return "";  // Valid
}

QString CategorySuggestionWizard::format_file_size(qint64 size) {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;

    if (size >= GB) {
        return QString::number(size / GB, 'f', 2) + " GB";
    } else if (size >= MB) {
        return QString::number(size / MB, 'f', 2) + " MB";
    } else if (size >= KB) {
        return QString::number(size / KB, 'f', 2) + " KB";
    } else {
        return QString::number(size) + " bytes";
    }
}

bool CategorySuggestionWizard::is_valid_path_segment(const std::string& segment) {
    // Check for invalid filesystem characters
    const std::string invalid_chars = "/<>:\"|?*\\";
    
    for (char c : segment) {
        if (invalid_chars.find(c) != std::string::npos) {
            return false;
        }
        // Also reject control characters
        if (c < 32) {
            return false;
        }
    }

    return true;
}

int CategorySuggestionWizard::count_path_depth(const std::string& path) {
    if (path.empty()) return 0;
    
    int depth = 1;
    for (char c : path) {
        if (c == '/') depth++;
    }
    return depth;
}
