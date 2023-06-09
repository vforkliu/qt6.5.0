/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -I QtCore/private/qglobal_p.h -p gattmanager1_p.h:gattmanager1.cpp org.bluez.GattManager1.xml --moc
 *
 * qdbusxml2cpp is Copyright (C) 2022 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef GATTMANAGER1_P_H
#define GATTMANAGER1_P_H

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
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>
#include <QtCore/private/qglobal_p.h>

/*
 * Proxy class for interface org.bluez.GattManager1
 */
class OrgBluezGattManager1Interface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.bluez.GattManager1"; }

public:
    OrgBluezGattManager1Interface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);

    ~OrgBluezGattManager1Interface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> RegisterApplication(const QDBusObjectPath &application, const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(application) << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QStringLiteral("RegisterApplication"), argumentList);
    }

    inline QDBusPendingReply<> UnregisterApplication(const QDBusObjectPath &application)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(application);
        return asyncCallWithArgumentList(QStringLiteral("UnregisterApplication"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

namespace org {
  namespace bluez {
    using GattManager1 = ::OrgBluezGattManager1Interface;
  }
}
#endif
