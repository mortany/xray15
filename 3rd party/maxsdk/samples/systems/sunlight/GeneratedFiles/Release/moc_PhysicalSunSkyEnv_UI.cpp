/****************************************************************************
** Meta object code from reading C++ file 'PhysicalSunSkyEnv_UI.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../PhysicalSunSkyEnv_UI.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PhysicalSunSkyEnv_UI.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.11.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PhysicalSunSkyEnv__MainPanelWidget_t {
    QByteArrayData data[3];
    char stringdata0[73];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PhysicalSunSkyEnv__MainPanelWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PhysicalSunSkyEnv__MainPanelWidget_t qt_meta_stringdata_PhysicalSunSkyEnv__MainPanelWidget = {
    {
QT_MOC_LITERAL(0, 0, 34), // "PhysicalSunSkyEnv::MainPanelW..."
QT_MOC_LITERAL(1, 35, 36), // "create_sun_positioner_button_..."
QT_MOC_LITERAL(2, 72, 0) // ""

    },
    "PhysicalSunSkyEnv::MainPanelWidget\0"
    "create_sun_positioner_button_clicked\0"
    ""
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PhysicalSunSkyEnv__MainPanelWidget[] = {

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
       1,    0,   19,    2, 0x09 /* Protected */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void PhysicalSunSkyEnv::MainPanelWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        MainPanelWidget *_t = static_cast<MainPanelWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->create_sun_positioner_button_clicked(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject PhysicalSunSkyEnv::MainPanelWidget::staticMetaObject = {
    { &MaxSDK::QMaxParamBlockWidget::staticMetaObject, qt_meta_stringdata_PhysicalSunSkyEnv__MainPanelWidget.data,
      qt_meta_data_PhysicalSunSkyEnv__MainPanelWidget,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *PhysicalSunSkyEnv::MainPanelWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PhysicalSunSkyEnv::MainPanelWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PhysicalSunSkyEnv__MainPanelWidget.stringdata0))
        return static_cast<void*>(this);
    return MaxSDK::QMaxParamBlockWidget::qt_metacast(_clname);
}

int PhysicalSunSkyEnv::MainPanelWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
