/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kplugintradertest.h"
#include "nsaplugin.h"

#include <QTest>

#include <kplugininfo.h>
#include <kplugintrader.h>

QTEST_MAIN(PluginTraderTest)

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

void PluginTraderTest::initTestCase()
{
    QCoreApplication::setLibraryPaths(QStringList() << QCoreApplication::applicationDirPath());
}

void PluginTraderTest::findPlugin_data()
{
    QTest::addColumn<QString>("serviceType");
    QTest::addColumn<QString>("constraint");
    QTest::addColumn<int>("expectedResult");

    const QString _st = QStringLiteral("KService/NSA");
    QString _c;
    QTest::newRow("no constraints") << _st << _c << 1;

    _c = QStringLiteral("[X-KDE-PluginInfo-Name] == '%1'").arg(QStringLiteral("fakeplugin"));
    QTest::newRow("by pluginname") << _st << _c << 1;

    _c = QStringLiteral("[X-KDE-PluginInfo-Category] == '%1'").arg(QStringLiteral("Examples"));
    QTest::newRow("by category") << _st << _c << 1;

    _c = QStringLiteral("([X-KDE-PluginInfo-Category] == 'Examples') AND ([X-KDE-PluginInfo-Email] == 'sebas@kde.org')");
    QTest::newRow("complex query") << _st << _c << 1;

    _c = QStringLiteral("([X-KDE-PluginInfo-Category] == 'Examples') AND ([X-KDE-PluginInfo-Email] == 'prrrrt')");
    QTest::newRow("empty query") << _st << _c << 0;
}

void PluginTraderTest::findPlugin()
{
    QFETCH(QString, serviceType);
    QFETCH(QString, constraint);
    QFETCH(int, expectedResult);
    const KPluginInfo::List res = KPluginTrader::self()->query(QString(), serviceType, constraint);
    QCOMPARE(res.count(), expectedResult);
}

void PluginTraderTest::findSomething()
{
    const KPluginInfo::List res = KPluginTrader::self()->query(QString());
    QVERIFY(res.count() > 0);
}

void PluginTraderTest::loadPlugin()
{
    const QString pluginName(QStringLiteral("fakeplugin"));
    const QString serviceType(QStringLiteral("KService/NSA"));
    const QString constraint = QStringLiteral("[X-KDE-PluginInfo-Name] == '%1'").arg(pluginName);

    QObject *plugin = KPluginTrader::createInstanceFromQuery<QObject>(QString(), serviceType, constraint, this);
    QVERIFY(plugin != nullptr);
    QCOMPARE(plugin->objectName(), QStringLiteral("Test Plugin Spy"));
}
