/****************************************************************************
** Meta object code from reading C++ file 'WhitelistTreeEditor.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/WhitelistTreeEditor.hpp"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WhitelistTreeEditor.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_WhitelistTreeEditor_t {
    uint offsetsAndSizes[26];
    char stringdata0[20];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[19];
    char stringdata4[25];
    char stringdata5[15];
    char stringdata6[16];
    char stringdata7[17];
    char stringdata8[5];
    char stringdata9[7];
    char stringdata10[21];
    char stringdata11[16];
    char stringdata12[29];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_WhitelistTreeEditor_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_WhitelistTreeEditor_t qt_meta_stringdata_WhitelistTreeEditor = {
    {
        QT_MOC_LITERAL(0, 19),  // "WhitelistTreeEditor"
        QT_MOC_LITERAL(20, 15),  // "on_add_category"
        QT_MOC_LITERAL(36, 0),  // ""
        QT_MOC_LITERAL(37, 18),  // "on_add_subcategory"
        QT_MOC_LITERAL(56, 24),  // "on_add_child_to_selected"
        QT_MOC_LITERAL(81, 14),  // "on_remove_item"
        QT_MOC_LITERAL(96, 15),  // "on_item_changed"
        QT_MOC_LITERAL(112, 16),  // "QTreeWidgetItem*"
        QT_MOC_LITERAL(129, 4),  // "item"
        QT_MOC_LITERAL(134, 6),  // "column"
        QT_MOC_LITERAL(141, 20),  // "on_selection_changed"
        QT_MOC_LITERAL(162, 15),  // "on_mode_changed"
        QT_MOC_LITERAL(178, 28)   // "on_edit_shared_subcategories"
    },
    "WhitelistTreeEditor",
    "on_add_category",
    "",
    "on_add_subcategory",
    "on_add_child_to_selected",
    "on_remove_item",
    "on_item_changed",
    "QTreeWidgetItem*",
    "item",
    "column",
    "on_selection_changed",
    "on_mode_changed",
    "on_edit_shared_subcategories"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_WhitelistTreeEditor[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x08,    1 /* Private */,
       3,    0,   63,    2, 0x08,    2 /* Private */,
       4,    0,   64,    2, 0x08,    3 /* Private */,
       5,    0,   65,    2, 0x08,    4 /* Private */,
       6,    2,   66,    2, 0x08,    5 /* Private */,
      10,    0,   71,    2, 0x08,    8 /* Private */,
      11,    0,   72,    2, 0x08,    9 /* Private */,
      12,    0,   73,    2, 0x08,   10 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7, QMetaType::Int,    8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject WhitelistTreeEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_WhitelistTreeEditor.offsetsAndSizes,
    qt_meta_data_WhitelistTreeEditor,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_WhitelistTreeEditor_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<WhitelistTreeEditor, std::true_type>,
        // method 'on_add_category'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_add_subcategory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_add_child_to_selected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_remove_item'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_item_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QTreeWidgetItem *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_selection_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_mode_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_edit_shared_subcategories'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void WhitelistTreeEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WhitelistTreeEditor *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->on_add_category(); break;
        case 1: _t->on_add_subcategory(); break;
        case 2: _t->on_add_child_to_selected(); break;
        case 3: _t->on_remove_item(); break;
        case 4: _t->on_item_changed((*reinterpret_cast< std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 5: _t->on_selection_changed(); break;
        case 6: _t->on_mode_changed(); break;
        case 7: _t->on_edit_shared_subcategories(); break;
        default: ;
        }
    }
}

const QMetaObject *WhitelistTreeEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WhitelistTreeEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_WhitelistTreeEditor.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int WhitelistTreeEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
