/*
    SPDX-FileCopyrightText: 2026 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

#include <KApplicationTrader>
#include <KService>

#include <stdio.h>

using namespace Qt::Literals;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"findintent"_s);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Finds a service handling an XDG Intent"_s);
    parser.addHelpOption();
    parser.addPositionalArgument(u"intent"_s, u"XDG Intent"_s);
    QCommandLineOption scopeOpt({u"s"_s, u"scope"_s}, u"Intent scope (optional)"_s, u"scope"_s);
    parser.addOption(scopeOpt);
    QCommandLineOption writeOpt({u"w"_s, u"write"_s}, u"Service name to set as the default"_s, u"desktop name"_s);
    parser.addOption(writeOpt);
    parser.process(app);

    if (parser.positionalArguments().size() != 1) {
        QTextStream(stderr) << "Exactly one Intent identifier required\n";
        parser.showHelp(1);
    }

    if (parser.isSet(writeOpt)) {
        KApplicationTrader::setPreferredServiceForIntent(parser.positionalArguments().at(0),
                                                         KService::serviceByDesktopName(parser.value(writeOpt)),
                                                         parser.value(scopeOpt));
        return 0;
    }

    const auto list = KApplicationTrader::queryByIntent(parser.positionalArguments().at(0), parser.value(scopeOpt));
    if (list.isEmpty()) {
        QTextStream(stdout) << "No service found.\n";
        return 1;
    }
    QTextStream(stdout) << "All services:\n";
    for (const auto &service : list) {
        QTextStream(stdout) << "  " << service->desktopEntryName() << "\n";
    }

    const auto pref = KApplicationTrader::preferredServiceForIntent(parser.positionalArguments().at(0), parser.value(scopeOpt));
    QTextStream(stdout) << "\nPreferred service: " << (pref ? pref->desktopEntryName() : u"<none>"_s) << "\n";
    return 0;
}
