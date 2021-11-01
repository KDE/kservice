/*
    SPDX-FileCopyrightText: 2012 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLUGINTEST_H
#define PLUGINTEST_H

#include <QObject>

class PluginTestPrivate;

class PluginTest : public QObject
{
    Q_OBJECT

public:
    PluginTest();
    ~PluginTest() override;

public Q_SLOTS:
    int runMain();
    bool loadFromKService(const QString &name = QStringLiteral("time"));
    bool loadFromMetaData(const QString &serviceType = QStringLiteral("Plasma/DataEngine"));
    bool findPlugins();
    void report(const QList<qint64> timings, const QString &msg = QStringLiteral("Test took "));

private:
    PluginTestPrivate *d;
};

#endif
