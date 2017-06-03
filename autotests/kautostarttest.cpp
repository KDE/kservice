/* This file is part of the KDE libraries
    Copyright (c) 2005 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kautostarttest.h"

#include <qstandardpaths.h>
#include <QTest>

#include <QtCore/QFile>

QTEST_MAIN(KAutostartTest)   // Qt5 TODO: QTEST_GUILESS_MAIN

#include <kautostart.h>

void KAutostartTest::testStartDetection_data()
{
    QTest::addColumn<QString>("service");
    QTest::addColumn<bool>("doesAutostart");
    if (KAutostart::isServiceRegistered(QStringLiteral("plasma-desktop"))) {
        QTest::newRow("plasma-desktop") << "plasma-desktop" << true;
    }
    if (KAutostart::isServiceRegistered(QStringLiteral("khotkeys"))) {
        QTest::newRow("khotkeys") << "khotkeys" << false;
    }
    QTest::newRow("does not exist") << "doesnotexist" << false;
}

void KAutostartTest::testStartDetection()
{
    QFETCH(QString, service);
    QFETCH(bool, doesAutostart);

    KAutostart autostart(service);
    QCOMPARE(autostart.autostarts(), doesAutostart);
}

void KAutostartTest::testStartInEnvDetection_data()
{
    QTest::addColumn<QString>("env");
    QTest::addColumn<bool>("doesAutostart");
    QTest::newRow("kde") << "KDE" << true;
    QTest::newRow("xfce") << "XFCE" << false;
}

void KAutostartTest::testStartInEnvDetection()
{
    QFETCH(QString, env);
    QFETCH(bool, doesAutostart);

    KAutostart autostart(QStringLiteral("plasma-desktop"));
    // Let's see if plasma.desktop actually exists
    if (QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QLatin1String("autostart/") + "plasma-desktop.desktop").isEmpty()) {
        QSKIP("plasma-desktop.desktop not found, kdebase not installed");
    } else {
        QCOMPARE(autostart.autostarts(env), doesAutostart);
    }
}

void KAutostartTest::testStartphase_data()
{
    QTest::addColumn<QString>("service");
    QTest::addColumn<int>("startPhase");
    if (KAutostart::isServiceRegistered(QStringLiteral("plasma-desktop"))) {
        QTest::newRow("plasma-desktop") << "plasma-desktop" << int(KAutostart::BaseDesktop);
    }
    if (KAutostart::isServiceRegistered(QStringLiteral("klipper"))) {
        QTest::newRow("klipper") << "klipper" << int(KAutostart::Applications);
    }
    if (KAutostart::isServiceRegistered(QStringLiteral("khotkeys"))) {
        QTest::newRow("khotkeys") << "ktip" << int(KAutostart::Applications);
    }
    QTest::newRow("does not exist") << "doesnotexist"
                                    << int(KAutostart::Applications);
}

void KAutostartTest::testStartphase()
{
    QFETCH(QString, service);
    QFETCH(int, startPhase);

    KAutostart autostart(service);
    QCOMPARE(int(autostart.startPhase()), startPhase);
}

void KAutostartTest::testStartName()
{
    if (!KAutostart::isServiceRegistered(QStringLiteral("plasma-desktop"))) {
        QSKIP("plasma-desktop.desktop not found, kdebase not installed");
    }
    KAutostart autostart(QStringLiteral("plasma-desktop"));
    QCOMPARE(autostart.visibleName(), QStringLiteral("Plasma Desktop Workspace"));
}

void KAutostartTest::testServiceRegistered()
{
    KAutostart autostart;
    QCOMPARE(KAutostart::isServiceRegistered(QStringLiteral("doesnotexist")), false);

    if (QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QLatin1String("autostart/") + "plasma-desktop.desktop").isEmpty()) {
        QSKIP("plasma-desktop.desktop not found, kdebase not installed");
    }
    QCOMPARE(KAutostart::isServiceRegistered(QStringLiteral("plasma-desktop")), true);
}

void KAutostartTest::testRegisteringAndManipulatingANewService()
{
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/autostart/") + "doesnotexist.desktop");
    {
        // need to clean up the KAutostart object before QFile can remove it
        KAutostart autostart(QStringLiteral("doesnotexist"));
        QCOMPARE(autostart.autostarts(), false);
        autostart.setCommand(QStringLiteral("aseigo"));
#ifdef Q_OS_WIN
        autostart.setCommandToCheck(QStringLiteral("cmd"));
#else
        autostart.setCommandToCheck(QStringLiteral("/bin/ls"));
#endif
        autostart.setVisibleName(QStringLiteral("doesnotexisttest"));
        autostart.setStartPhase(KAutostart::BaseDesktop);
        autostart.setAllowedEnvironments(QStringList(QStringLiteral("KDE")));
        autostart.addToAllowedEnvironments(QStringLiteral("XFCE"));
        autostart.addToAllowedEnvironments(QStringLiteral("GNOME"));
        autostart.removeFromAllowedEnvironments(QStringLiteral("GNOME"));
        autostart.setExcludedEnvironments(QStringList(QStringLiteral("GNOME")));
        autostart.addToExcludedEnvironments(QStringLiteral("XFCE"));
        autostart.addToExcludedEnvironments(QStringLiteral("KDE"));
        autostart.removeFromExcludedEnvironments(QStringLiteral("KDE"));
    }

    QVERIFY(QFile::exists(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/autostart/") + "doesnotexist.desktop"));

    {
        QStringList allowedEnvs;
        allowedEnvs << QStringLiteral("KDE") << QStringLiteral("XFCE");

        QStringList excludedEnvs;
        excludedEnvs << QStringLiteral("GNOME") << QStringLiteral("XFCE");

        KAutostart autostart(QStringLiteral("doesnotexist"));
        QCOMPARE(autostart.command(), QStringLiteral("aseigo"));
        QCOMPARE(autostart.autostarts(), true);
        QCOMPARE(autostart.autostarts(QStringLiteral("KDE")), true);
        QCOMPARE(autostart.autostarts(QStringLiteral("GNOME")), false);
        QCOMPARE(autostart.autostarts(QStringLiteral("XFCE")), true);
        QCOMPARE(autostart.autostarts(QStringLiteral("XFCE"), KAutostart::CheckCommand), true);
        QCOMPARE(autostart.visibleName(), QStringLiteral("doesnotexisttest"));
        QCOMPARE(autostart.commandToCheck(), QStringLiteral("/bin/ls"));
        QCOMPARE(int(autostart.startPhase()), int(KAutostart::BaseDesktop));
        QCOMPARE(autostart.allowedEnvironments(), allowedEnvs);
        QCOMPARE(autostart.excludedEnvironments(), excludedEnvs);

        autostart.setCommandToCheck(QStringLiteral("/bin/whozitwhat"));
    }

    {
        KAutostart autostart(QStringLiteral("doesnotexist"));
        QCOMPARE(autostart.autostarts(QStringLiteral("KDE"), KAutostart::CheckCommand), false);
    }
}

void KAutostartTest::testRemovalOfNewServiceFile()
{
    QVERIFY(QFile::remove(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/autostart/") + "doesnotexist.desktop"));
}

