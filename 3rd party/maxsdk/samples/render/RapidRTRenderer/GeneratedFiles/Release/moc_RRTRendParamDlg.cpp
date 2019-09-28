/****************************************************************************
** Meta object code from reading C++ file 'RRTRendParamDlg.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../RRTRendParamDlg.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'RRTRendParamDlg.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.11.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Max__RapidRTTranslator__RenderParamsQtDialog_t {
    QByteArrayData data[4];
    char stringdata0[84];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Max__RapidRTTranslator__RenderParamsQtDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Max__RapidRTTranslator__RenderParamsQtDialog_t qt_meta_stringdata_Max__RapidRTTranslator__RenderParamsQtDialog = {
    {
QT_MOC_LITERAL(0, 0, 44), // "Max::RapidRTTranslator::Rende..."
QT_MOC_LITERAL(1, 45, 27), // "render_method_combo_changed"
QT_MOC_LITERAL(2, 73, 0), // ""
QT_MOC_LITERAL(3, 74, 9) // "new_index"

    },
    "Max::RapidRTTranslator::RenderParamsQtDialog\0"
    "render_method_combo_changed\0\0new_index"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Max__RapidRTTranslator__RenderParamsQtDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,

       0        // eod
};

void Max::RapidRTTranslator::RenderParamsQtDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        RenderParamsQtDialog *_t = static_cast<RenderParamsQtDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->render_method_combo_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Max::RapidRTTranslator::RenderParamsQtDialog::staticMetaObject = {
    { &MaxSDK::QMaxParamBlockWidget::staticMetaObject, qt_meta_stringdata_Max__RapidRTTranslator__RenderParamsQtDialog.data,
      qt_meta_data_Max__RapidRTTranslator__RenderParamsQtDialog,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *Max::RapidRTTranslator::RenderParamsQtDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Max::RapidRTTranslator::RenderParamsQtDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Max__RapidRTTranslator__RenderParamsQtDialog.stringdata0))
        return static_cast<void*>(this);
    return MaxSDK::QMaxParamBlockWidget::qt_metacast(_clname);
}

int Max::RapidRTTranslator::RenderParamsQtDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MaxSDK::QMaxParamBlockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_Max__RapidRTTranslator__ImageQualityQtDialog_t {
    QByteArrayData data[1];
    char stringdata0[45];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Max__RapidRTTranslator__ImageQualityQtDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Max__RapidRTTranslator__ImageQualityQtDialog_t qt_meta_stringdata_Max__RapidRTTranslator__ImageQualityQtDialog = {
    {
QT_MOC_LITERAL(0, 0, 44) // "Max::RapidRTTranslator::Image..."

    },
    "Max::RapidRTTranslator::ImageQualityQtDialog"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Max__RapidRTTranslator__ImageQualityQtDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void Max::RapidRTTranslator::ImageQualityQtDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject Max::RapidRTTranslator::ImageQualityQtDialog::staticMetaObject = {
    { &MaxSDK::QMaxParamBlockWidget::staticMetaObject, qt_meta_stringdata_Max__RapidRTTranslator__ImageQualityQtDialog.data,
      qt_meta_data_Max__RapidRTTranslator__ImageQualityQtDialog,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *Max::RapidRTTranslator::ImageQualityQtDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Max::RapidRTTranslator::ImageQualityQtDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Max__RapidRTTranslator__ImageQualityQtDialog.stringdata0))
        return static_cast<void*>(this);
    return MaxSDK::QMaxParamBlockWidget::qt_metacast(_clname);
}

int Max::RapidRTTranslator::ImageQualityQtDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MaxSDK::QMaxParamBlockWidget::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_Max__RapidRTTranslator__AdvancedParamsQtDialog_t {
    QByteArrayData data[1];
    char stringdata0[47];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Max__RapidRTTranslator__AdvancedParamsQtDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Max__RapidRTTranslator__AdvancedParamsQtDialog_t qt_meta_stringdata_Max__RapidRTTranslator__AdvancedParamsQtDialog = {
    {
QT_MOC_LITERAL(0, 0, 46) // "Max::RapidRTTranslator::Advan..."

    },
    "Max::RapidRTTranslator::AdvancedParamsQtDialog"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Max__RapidRTTranslator__AdvancedParamsQtDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void Max::RapidRTTranslator::AdvancedParamsQtDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject Max::RapidRTTranslator::AdvancedParamsQtDialog::staticMetaObject = {
    { &MaxSDK::QMaxParamBlockWidget::staticMetaObject, qt_meta_stringdata_Max__RapidRTTranslator__AdvancedParamsQtDialog.data,
      qt_meta_data_Max__RapidRTTranslator__AdvancedParamsQtDialog,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *Max::RapidRTTranslator::AdvancedParamsQtDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Max::RapidRTTranslator::AdvancedParamsQtDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Max__RapidRTTranslator__AdvancedParamsQtDialog.stringdata0))
        return static_cast<void*>(this);
    return MaxSDK::QMaxParamBlockWidget::qt_metacast(_clname);
}

int Max::RapidRTTranslator::AdvancedParamsQtDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MaxSDK::QMaxParamBlockWidget::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
