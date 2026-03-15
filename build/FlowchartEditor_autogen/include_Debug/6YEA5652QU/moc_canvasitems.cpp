/****************************************************************************
** Meta object code from reading C++ file 'canvasitems.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../include/canvasitems.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'canvasitems.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9flowchart14CanvasNodeItemE_t {};
} // unnamed namespace

template <> constexpr inline auto flowchart::CanvasNodeItem::qt_create_metaobjectdata<qt_meta_tag_ZN9flowchart14CanvasNodeItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "flowchart::CanvasNodeItem"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CanvasNodeItem, qt_meta_tag_ZN9flowchart14CanvasNodeItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject flowchart::CanvasNodeItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9flowchart14CanvasNodeItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9flowchart14CanvasNodeItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9flowchart14CanvasNodeItemE_t>.metaTypes,
    nullptr
} };

void flowchart::CanvasNodeItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CanvasNodeItem *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *flowchart::CanvasNodeItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *flowchart::CanvasNodeItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9flowchart14CanvasNodeItemE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "CanvasDiagramItem"))
        return static_cast< CanvasDiagramItem*>(this);
    return QGraphicsObject::qt_metacast(_clname);
}

int flowchart::CanvasNodeItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_tag_ZN9flowchart19CanvasConnectorItemE_t {};
} // unnamed namespace

template <> constexpr inline auto flowchart::CanvasConnectorItem::qt_create_metaobjectdata<qt_meta_tag_ZN9flowchart19CanvasConnectorItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "flowchart::CanvasConnectorItem"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CanvasConnectorItem, qt_meta_tag_ZN9flowchart19CanvasConnectorItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject flowchart::CanvasConnectorItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9flowchart19CanvasConnectorItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9flowchart19CanvasConnectorItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9flowchart19CanvasConnectorItemE_t>.metaTypes,
    nullptr
} };

void flowchart::CanvasConnectorItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CanvasConnectorItem *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *flowchart::CanvasConnectorItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *flowchart::CanvasConnectorItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9flowchart19CanvasConnectorItemE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "CanvasDiagramItem"))
        return static_cast< CanvasDiagramItem*>(this);
    return QGraphicsObject::qt_metacast(_clname);
}

int flowchart::CanvasConnectorItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
