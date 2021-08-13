/*
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <kdbusservicestarter.h>

#include <QCoreApplication>
#include <kservice.h>

int main(int argc, char **argv)
{
    QCoreApplication::setApplicationName(QStringLiteral("kdbusservicestartertest"));
    QCoreApplication app(argc, argv);

    QString error;
    QString dbusService;
    KDBusServiceStarter::self()->findServiceFor(QStringLiteral("DBUS/Organizer"), QString(), &error, &dbusService);

    return 0;
}
