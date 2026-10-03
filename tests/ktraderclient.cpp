/*
 *  SPDX-FileCopyrightText: 2026 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include <QCommandLineParser>

#include <KApplicationTrader>

using namespace Qt::Literals;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addPositionalArgument(u"mimetype"_s, u"The MIME type"_s);
    parser.addOption(QCommandLineOption(u"preferred"_s, u"List only the preferred service"_s));

    parser.process(app);

    if (parser.positionalArguments().length() != 1) {
        parser.showHelp();
    }

    if (parser.isSet(u"preferred"_s)) {
        const auto service = KApplicationTrader::preferredService(parser.positionalArguments().first());
        QTextStream(stdout) << service->desktopEntryName() << Qt::endl;
    } else {
        const auto list = KApplicationTrader::queryByMimeType(parser.positionalArguments().first());
        for (const auto &service : list) {
            QTextStream(stdout) << service->desktopEntryName() << Qt::endl;
        }
    }
}
