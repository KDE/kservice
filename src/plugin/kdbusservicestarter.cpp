/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kdbusservicestarter.h"
#include "kservice.h"
#include "kservicetypetrader.h"
#include <KLocalizedString>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>
#include <ktoolinvocation.h>

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 61)

class KDBusServiceStarterPrivate
{
public:
    KDBusServiceStarterPrivate()
        : q(nullptr)
    {
    }
    ~KDBusServiceStarterPrivate()
    {
        delete q;
    }
    KDBusServiceStarter *q;
};

Q_GLOBAL_STATIC(KDBusServiceStarterPrivate, privateObject)

KDBusServiceStarter *KDBusServiceStarter::self()
{
    if (!privateObject()->q) {
        new KDBusServiceStarter;
        Q_ASSERT(privateObject()->q);
    }
    return privateObject()->q;
}

KDBusServiceStarter::KDBusServiceStarter()
{
    // Set the singleton instance - useful when a derived KDBusServiceStarter
    // was created (before self() was called)
    Q_ASSERT(!privateObject()->q);
    privateObject()->q = this;
}

KDBusServiceStarter::~KDBusServiceStarter()
{
}

int KDBusServiceStarter::findServiceFor(const QString &serviceType, const QString &_constraint, QString *error, QString *pDBusService, int flags)
{
    // Ask the trader which service is preferred for this servicetype
    // We want one that provides a DBus interface
    QString constraint = _constraint;
    if (!constraint.isEmpty()) {
        constraint += QLatin1String(" and ");
    }
    constraint += QLatin1String("exist [X-DBUS-ServiceName]");
    const KService::List offers = KServiceTypeTrader::self()->query(serviceType, constraint);
    if (offers.isEmpty()) {
        if (error) {
            *error = i18n("No service implementing %1", serviceType);
        }
        qWarning() << "KDBusServiceStarter: No service implementing " << serviceType;
        return -1;
    }
    KService::Ptr ptr = offers.first();
    QString dbusService = ptr->property(QStringLiteral("X-DBUS-ServiceName")).toString();

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(dbusService)) {
        QString err;
        if (startServiceFor(serviceType, constraint, &err, &dbusService, flags) != 0) {
            if (error) {
                *error = err;
            }
            qWarning() << "Couldn't start service" << dbusService << "for" << serviceType << ":" << err;
            return -2;
        }
    }
    // qDebug() << "DBus service is available now, as" << dbusService;
    if (pDBusService) {
        *pDBusService = dbusService;
    }
    return 0;
}

int KDBusServiceStarter::startServiceFor(const QString &serviceType, const QString &constraint, QString *error, QString *dbusService, int /*flags*/)
{
    const KService::List offers = KServiceTypeTrader::self()->query(serviceType, constraint);
    if (offers.isEmpty()) {
        return -1;
    }
    KService::Ptr ptr = offers.first();
    // qDebug() << "starting" << ptr->entryPath();
    return KToolInvocation::startServiceByDesktopPath(ptr->entryPath(), QStringList(), error, dbusService);
}

#endif
