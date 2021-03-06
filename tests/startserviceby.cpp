/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include <QCoreApplication>
#include <QDebug>
#include <ktoolinvocation.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString serviceId = QStringLiteral("kwrite.desktop");
    if (argc > 1) {
        serviceId = QString::fromLocal8Bit(argv[1]);
    }
    QString url;
    if (argc > 2) {
        url = QString::fromLocal8Bit(argv[2]);
    }

    QString error;
    QString dbusService;
    int pid;
    KToolInvocation::startServiceByDesktopPath(serviceId, url, &error, &dbusService, &pid);
    qDebug() << "Started. error=" << error << " dbusService=" << dbusService << " pid=" << pid;

    return 0;
}
