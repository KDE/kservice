/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLUGINTRADERTEST_H
#define PLUGINTRADERTEST_H

#include <QCoreApplication>

class PluginTraderTest : public QObject
{
    Q_OBJECT
public:
    PluginTraderTest()
    {
    }

private Q_SLOTS:
    void initTestCase();
    void findPlugin_data();
    void findPlugin();
    void findSomething();
    void loadPlugin();
};

#endif
