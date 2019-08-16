/*
 *  Copyright (C) 2016 David Faure   <faure@kde.org>
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

#include <kservice.h>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <kmimeassociations_p.h>
#include <ksycoca_p.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Parses mimeapps.list files and reports results for a mimetype"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("mime"), QStringLiteral("mimetype name"));
    parser.process(app);

    if (parser.positionalArguments().count() != 1) {
        QTextStream(stderr) << "Exactly one mimetype required\n";
        parser.showHelp(1);
    }

    const QString mime = parser.positionalArguments().at(0);

    // To see which files are being parsed, run this command:
    //    strace -e file ./kmimeassociations_dumper inode/directory |& grep mimeapps

    // It should be the same as the output of these commands (assuming $XDG_CURRENT_DESKTOP=KDE)
    // qtpaths --locate-files GenericConfigLocation kde-mimeapps.list
    // qtpaths --locate-files GenericConfigLocation mimeapps.list
    // qtpaths --locate-files ApplicationsLocation kde-mimeapps.list
    // qtpaths --locate-files ApplicationsLocation mimeapps.list

    KOfferHash offers;
    KMimeAssociations mimeAppsParser(offers, KSycocaPrivate::self()->serviceFactory());
    mimeAppsParser.parseAllMimeAppsList();

    QTextStream output(stdout);
    const auto list = offers.offersFor(mime);
    for (const KServiceOffer &offer : list) {
        output << offer.service()->desktopEntryName() << " " << offer.service()->entryPath() << "\n";
    }

    return 0;
}
