/*
 * Copyright 2014 Alex Merry <alex.merry@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

#include <KService>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("findservice"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Finds a service using KService"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("id"), QStringLiteral("service identifier"));
    QCommandLineOption desktopName(
        QStringList() << QStringLiteral("n") << QStringLiteral("desktop-name"),
        QStringLiteral("Find the service by its desktop name (default)"));
    parser.addOption(desktopName);
    QCommandLineOption desktopPath(
        QStringList() << QStringLiteral("p") << QStringLiteral("desktop-path"),
        QStringLiteral("Find the service by its desktop path"));
    parser.addOption(desktopPath);
    QCommandLineOption menuId(
        QStringList() << QStringLiteral("m") << QStringLiteral("menu-id"),
        QStringLiteral("Find the service by its menu id"));
    parser.addOption(menuId);
    QCommandLineOption storageId(
        QStringList() << QStringLiteral("s") << QStringLiteral("storage-id"),
        QStringLiteral("Find the service by its storage id"));
    parser.addOption(storageId);

    parser.process(app);

    if (parser.positionalArguments().count() != 1) {
        QTextStream(stderr) << "Exactly one identifier required\n";
        parser.showHelp(1);
    }

    const QString id = parser.positionalArguments().at(0);

    KService::Ptr service;
    if (parser.isSet(menuId)) {
        service = KService::serviceByMenuId(id);
    } else if (parser.isSet(storageId)) {
        service = KService::serviceByStorageId(id);
    } else if (parser.isSet(desktopPath)) {
        service = KService::serviceByDesktopPath(id);
    } else {
        service = KService::serviceByDesktopName(id);
    }

    if (service) {
        QTextStream(stdout) << "Found \"" << service->entryPath() << "\"\n";
        QTextStream(stdout) << "Desktop name: \"" << service->desktopEntryName() << "\"\n";
        QTextStream(stdout) << "Menu ID: \"" << service->menuId() << "\"\n";
        QTextStream(stdout) << "Storage ID: \"" << service->storageId() << "\"\n";
    } else {
        QTextStream(stdout) << "Not found\n";
        return 2;
    }

    return 0;
}
