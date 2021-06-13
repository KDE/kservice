/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QCommandLineParser>

#include "plugintest.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;

    const QString description = QStringLiteral("KPluginTrader test app");
    const char version[] = "1.0";

    app.setApplicationVersion(QString::fromLatin1(version));
    parser.addVersionOption();
    parser.addHelpOption();
    parser.setApplicationDescription(description);

    PluginTest test;
    return test.runMain();
}
