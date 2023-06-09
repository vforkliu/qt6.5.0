/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -a propertiesadaptor_p.h:propertiesadaptor.cpp -c OrgFreedesktopDBusPropertiesAdaptor -i bluez5_helper_p.h org.freedesktop.dbus.properties.xml
 *
 * qdbusxml2cpp is Copyright (C) 2022 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef PROPERTIESADAPTOR_P_H
#define PROPERTIESADAPTOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include "bluez5_helper_p.h"
#include <QtCore/qcontainerfwd.h>

/*
 * Adaptor class for interface org.freedesktop.DBus.Properties
 */
class OrgFreedesktopDBusPropertiesAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.DBus.Properties")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.DBus.Properties\">\n"
"    <method name=\"Get\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"interface\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\"/>\n"
"      <arg direction=\"out\" type=\"v\" name=\"value\"/>\n"
"    </method>\n"
"    <method name=\"Set\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"interface\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\"/>\n"
"      <arg direction=\"in\" type=\"v\" name=\"value\"/>\n"
"    </method>\n"
"    <method name=\"GetAll\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"interface\"/>\n"
"      <arg direction=\"out\" type=\"a{sv}\" name=\"properties\"/>\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"    </method>\n"
"    <signal name=\"PropertiesChanged\">\n"
"      <arg type=\"s\" name=\"interface\"/>\n"
"      <arg type=\"a{sv}\" name=\"changed_properties\"/>\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out1\"/>\n"
"      <arg type=\"as\" name=\"invalidated_properties\"/>\n"
"    </signal>\n"
"  </interface>\n"
        "")
public:
    OrgFreedesktopDBusPropertiesAdaptor(QObject *parent);
    virtual ~OrgFreedesktopDBusPropertiesAdaptor();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    QDBusVariant Get(const QString &interface, const QString &name);
    QVariantMap GetAll(const QString &interface);
    void Set(const QString &interface, const QString &name, const QDBusVariant &value);
Q_SIGNALS: // SIGNALS
    void PropertiesChanged(const QString &interface, const QVariantMap &changed_properties, const QStringList &invalidated_properties);
};

#endif
