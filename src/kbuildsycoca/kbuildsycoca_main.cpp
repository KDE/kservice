/*  This file is part of the KDE libraries
 *  Copyright (C) 1999 David Faure <faure@kde.org>
 *  Copyright (C) 2002-2003 Waldo Bastian <bastian@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation;
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <kbuildsycoca_p.h>

#include "../../kservice_version.h"

#include <klocalizedstring.h>
#include <kaboutdata.h>
#include <kcrash.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>

#include <qplatformdefs.h> // for unlink

static void crashHandler(int)
{
    // If we crash while reading sycoca, we delete the database
    // in an attempt to recover.
    if (KBuildSycoca::sycocaPath()) {
        unlink(KBuildSycoca::sycocaPath());
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kservice5");

    KAboutData about(QStringLiteral(KBUILDSYCOCA_EXENAME),
                     i18nc("application name", "KBuildSycoca"),
                     QStringLiteral(KSERVICE_VERSION_STRING),
                     i18nc("application description", "Rebuilds the system configuration cache."),
                     KAboutLicense::GPL,
                     i18nc("@info:credit", "Copyright 1999-2014 KDE Developers"));
    about.addAuthor(i18nc("@info:credit", "David Faure"),
                    i18nc("@info:credit", "Author"),
                    QStringLiteral("faure@kde.org"));
    about.addAuthor(i18nc("@info:credit", "Waldo Bastian"),
                    i18nc("@info:credit", "Author"),
                    QStringLiteral("bastian@kde.org"));
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringLiteral("nosignal"),
                i18nc("@info:shell command-line option",
                      "Do not signal applications to update (deprecated, no longer having any effect)")));
    parser.addOption(QCommandLineOption(QStringLiteral("noincremental"),
                i18nc("@info:shell command-line option",
                      "Disable incremental update, re-read everything")));
    parser.addOption(QCommandLineOption(QStringLiteral("checkstamps"),
                i18nc("@info:shell command-line option",
                      "Check file timestamps (deprecated, no longer having any effect)")));
    parser.addOption(QCommandLineOption(QStringLiteral("nocheckfiles"),
                i18nc("@info:shell command-line option",
                      "Disable checking files (deprecated, no longer having any effect)")));
    parser.addOption(QCommandLineOption(QStringLiteral("menutest"),
                i18nc("@info:shell command-line option",
                      "Perform menu generation test run only")));
    parser.addOption(QCommandLineOption(QStringLiteral("track"),
                i18nc("@info:shell command-line option",
                      "Track menu id for debug purposes"),
                QStringLiteral("menu-id")));
    parser.addOption(QCommandLineOption(QStringLiteral("testmode"),
                i18nc("@info:shell command-line option",
                      "Switch QStandardPaths to test mode, for unit tests only")));
    parser.process(app);
    about.processCommandLine(&parser);

    const bool bMenuTest = parser.isSet(QStringLiteral("menutest"));

    if (parser.isSet(QStringLiteral("testmode"))) {
        QStandardPaths::setTestModeEnabled(true);
    }

    KCrash::setEmergencySaveFunction(crashHandler);

    fprintf(stderr, "%s running...\n", KBUILDSYCOCA_EXENAME);

    const bool incremental = !parser.isSet(QStringLiteral("noincremental"));

    KBuildSycoca sycoca; // Build data base
    if (parser.isSet(QStringLiteral("track"))) {
        sycoca.setTrackId(parser.value(QStringLiteral("track")));
    }
    sycoca.setMenuTest(bMenuTest);
    if (!sycoca.recreate(incremental)) {
        return -1;
    }

    return 0;
}
