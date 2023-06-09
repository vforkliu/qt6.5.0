/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -a propertiesadaptor_p.h:propertiesadaptor.cpp -c OrgFreedesktopDBusPropertiesAdaptor -i bluez5_helper_p.h org.freedesktop.dbus.properties.xml
 *
 * qdbusxml2cpp is Copyright (C) 2022 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#include "propertiesadaptor_p.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class OrgFreedesktopDBusPropertiesAdaptor
 */

OrgFreedesktopDBusPropertiesAdaptor::OrgFreedesktopDBusPropertiesAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

OrgFreedesktopDBusPropertiesAdaptor::~OrgFreedesktopDBusPropertiesAdaptor()
{
    // destructor
}

QDBusVariant OrgFreedesktopDBusPropertiesAdaptor::Get(const QString &interface, const QString &name)
{
    // handle method call org.freedesktop.DBus.Properties.Get
    QDBusVariant value;
    QMetaObject::invokeMethod(parent(), "Get", Q_RETURN_ARG(QDBusVariant, value), Q_ARG(QString, interface), Q_ARG(QString, name));
    return value;
}

QVariantMap OrgFreedesktopDBusPropertiesAdaptor::GetAll(const QString &interface)
{
    // handle method call org.freedesktop.DBus.Properties.GetAll
    QVariantMap properties;
    QMetaObject::invokeMethod(parent(), "GetAll", Q_RETURN_ARG(QVariantMap, properties), Q_ARG(QString, interface));
    return properties;
}

void OrgFreedesktopDBusPropertiesAdaptor::Set(const QString &interface, const QString &name, const QDBusVariant &value)
{
    // handle method call org.freedesktop.DBus.Properties.Set
    QMetaObject::invokeMethod(parent(), "Set", Q_ARG(QString, interface), Q_ARG(QString, name), Q_ARG(QDBusVariant, value));
}
