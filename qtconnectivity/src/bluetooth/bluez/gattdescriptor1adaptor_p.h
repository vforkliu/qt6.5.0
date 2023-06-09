/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -a gattdescriptor1adaptor_p.h:gattdescriptor1adaptor.cpp -c OrgBluezGattDescriptor1Adaptor -i bluez5_helper_p.h org.bluez.GattDescriptor1.xml
 *
 * qdbusxml2cpp is Copyright (C) 2022 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef GATTDESCRIPTOR1ADAPTOR_P_H
#define GATTDESCRIPTOR1ADAPTOR_P_H

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
 * Adaptor class for interface org.bluez.GattDescriptor1
 */
class OrgBluezGattDescriptor1Adaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.bluez.GattDescriptor1")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.bluez.GattDescriptor1\">\n"
"    <method name=\"ReadValue\">\n"
"      <arg direction=\"in\" type=\"a{sv}\" name=\"options\"/>\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.In0\"/>\n"
"      <arg direction=\"out\" type=\"ay\" name=\"value\"/>\n"
"    </method>\n"
"    <method name=\"WriteValue\">\n"
"      <arg direction=\"in\" type=\"ay\" name=\"value\"/>\n"
"      <arg direction=\"in\" type=\"a{sv}\" name=\"options\"/>\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.In1\"/>\n"
"    </method>\n"
"    <property access=\"read\" type=\"s\" name=\"UUID\"/>\n"
"    <property access=\"read\" type=\"o\" name=\"Characteristic\"/>\n"
"    <property access=\"read\" type=\"ay\" name=\"Value\"/>\n"
"  </interface>\n"
        "")
public:
    OrgBluezGattDescriptor1Adaptor(QObject *parent);
    virtual ~OrgBluezGattDescriptor1Adaptor();

public: // PROPERTIES
    Q_PROPERTY(QDBusObjectPath Characteristic READ characteristic)
    QDBusObjectPath characteristic() const;

    Q_PROPERTY(QString UUID READ uUID)
    QString uUID() const;

    Q_PROPERTY(QByteArray Value READ value)
    QByteArray value() const;

public Q_SLOTS: // METHODS
    QByteArray ReadValue(const QVariantMap &options, const QDBusMessage &msg);
    void WriteValue(const QByteArray &value, const QVariantMap &options, const QDBusMessage& msg);
Q_SIGNALS: // SIGNALS
};

#endif
