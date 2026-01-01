/****************************************************************************
** Meta object code from reading C++ file 'CacheManagerDialog.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/CacheManagerDialog.hpp"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CacheManagerDialog.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_CacheManagerDialog_t {
    uint offsetsAndSizes[12];
    char stringdata0[19];
    char stringdata1[21];
    char stringdata2[1];
    char stringdata3[21];
    char stringdata4[20];
    char stringdata5[19];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CacheManagerDialog_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CacheManagerDialog_t qt_meta_stringdata_CacheManagerDialog = {
    {
        QT_MOC_LITERAL(0, 18),  // "CacheManagerDialog"
        QT_MOC_LITERAL(19, 20),  // "on_clear_all_clicked"
        QT_MOC_LITERAL(40, 0),  // ""
        QT_MOC_LITERAL(41, 20),  // "on_clear_old_clicked"
        QT_MOC_LITERAL(62, 19),  // "on_optimize_clicked"
        QT_MOC_LITERAL(82, 18)   // "on_refresh_clicked"
    },
    "CacheManagerDialog",
    "on_clear_all_clicked",
    "",
    "on_clear_old_clicked",
    "on_optimize_clicked",
    "on_refresh_clicked"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CacheManagerDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x08,    1 /* Private */,
       3,    0,   39,    2, 0x08,    2 /* Private */,
       4,    0,   40,    2, 0x08,    3 /* Private */,
       5,    0,   41,    2, 0x08,    4 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject CacheManagerDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_CacheManagerDialog.offsetsAndSizes,
    qt_meta_data_CacheManagerDialog,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CacheManagerDialog_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CacheManagerDialog, std::true_type>,
        // method 'on_clear_all_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_clear_old_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_optimize_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_refresh_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void CacheManagerDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CacheManagerDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->on_clear_all_clicked(); break;
        case 1: _t->on_clear_old_clicked(); break;
        case 2: _t->on_optimize_clicked(); break;
        case 3: _t->on_refresh_clicked(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *CacheManagerDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CacheManagerDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CacheManagerDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CacheManagerDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
