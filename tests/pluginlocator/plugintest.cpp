/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "plugintest.h"

#include <QDebug>
#include <kplugininfo.h>
#include <kplugintrader.h>
#include <kservice.h>
#include <kservicetypetrader.h>

#include <QElapsedTimer>
#include <QStandardPaths>
#include <QStringList>

static QTextStream cout(stdout); // clazy:exclude=non-pod-global-static

class PluginTestPrivate
{
public:
    QString pluginName;
};

PluginTest::PluginTest()
    : QObject(nullptr)
{
    d = new PluginTestPrivate;
}

PluginTest::~PluginTest()
{
    delete d;
}

int PluginTest::runMain()
{
    // measure performance
    QElapsedTimer timer;
    int runs = 1;
    QList<qint64> timings;

    cout << "-- KPluginTrader Test --" << Qt::endl;
    bool ok = true;

    // KSycoca querying
    timer.start();

    for (int _i = 0; _i < runs; _i++) {
        timer.restart();
        if (!loadFromKService(QStringLiteral("time"))) {
            ok = false;
        }
        timings << timer.nsecsElapsed();
    }
    report(timings, QStringLiteral("KServiceTypeTrader"));
    timings.clear();

    // -- Metadata querying
    for (int _i = 0; _i < runs; _i++) {
        timer.restart();
        if (!loadFromMetaData()) {
            ok = false;
        }
        // if (!loadFromMetaData2("Plasma/ContainmentActions")) ok = false;
        timings << timer.nsecsElapsed();
    }
    report(timings, QStringLiteral("Metadata"));
    timings.clear();

    findPlugins();

    if (ok) {
        qDebug() << "All tests finished successfully";
        return 0;
    }
    return 0; // We return successfully in any case, since plugins aren't installed for most people
}

void PluginTest::report(const QList<qint64> timings, const QString &msg)
{
    qulonglong totalTime = 0;

    int unitDiv = 1000;
    QString unit = QStringLiteral("microsec");
    int i = 0;
    for (qint64 t : timings) {
        int msec = t / 1000000;
        qDebug() << "  Run " << i << ": " << msec << " msec";
        totalTime += t;
        i++;
    }
    QString av = QString::number((totalTime / timings.count() / unitDiv), 'f', 1);
    qDebug() << " Average: " << av << " " << unit << " (" << msg << ")";
}

bool PluginTest::loadFromKService(const QString &name)
{
    bool ok = false;
    QString constraint = QStringLiteral("[X-KDE-PluginInfo-Name] == '%1'").arg(name);
    KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("Plasma/DataEngine"), constraint);
    if (offers.isEmpty()) {
        qDebug() << "offers are empty for " << name << " with constraint " << constraint;
    } else {
        QVariantList allArgs;
        allArgs << offers.first()->storageId();
        const QString _n = offers.first()->property(QStringLiteral("Name")).toString();
        if (!_n.isEmpty()) {
            qDebug() << "Found Dataengine: " << _n;
            ok = true;
        } else {
            qDebug() << "Nothing found. ";
        }
    }

    return ok;
}

bool PluginTest::loadFromMetaData(const QString &serviceType)
{
    bool ok = false;
    QString pluginName(QStringLiteral("time"));
    QString constraint = QStringLiteral("[X-KDE-PluginInfo-Name] == '%1'").arg(pluginName);
    const KPluginInfo::List res = KPluginTrader::self()->query(QStringLiteral("kf5"), serviceType, QString());
    qDebug() << "----------- Found " << res.count() << " Plugins" << constraint;
    ok = res.count() > 0;
    for (const KPluginInfo &info : res) {
        qDebug() << "   file: " << info.libraryPath();
    }

    return ok;
}

bool PluginTest::findPlugins()
{
    QElapsedTimer timer;
    QList<qint64> timings;
    const QString pluginDir(QStringLiteral("/media/storage/testdata/"));
    const QStringList sizes = QStringList() << QStringLiteral("50") << QStringLiteral("100") << QStringLiteral("150") << QStringLiteral("200")
                                            << QStringLiteral("250") << QStringLiteral("300") << QStringLiteral("400") << QStringLiteral("500")
                                            << QStringLiteral("600") << QStringLiteral("700") << QStringLiteral("800") << QStringLiteral("1000")
                                            << QStringLiteral("1500") << QStringLiteral("2000") << QStringLiteral("5000");
    QStringList datadirs;

    for (const QString &_s : sizes) {
        datadirs << pluginDir + _s;
    }
    for (const QString &subdir : std::as_const(datadirs)) {
        const QString constraint;
        const QString serviceType;

        timer.restart();
        KPluginInfo::List res = KPluginTrader::self()->query(subdir, serviceType, constraint);
        timings << timer.nsecsElapsed();
        qDebug() << "Found " << res.count() << " Plugins in " << subdir;
    }
    report(timings, QStringLiteral("reading monsterdirs"));
    return true;
}

#include "moc_plugintest.cpp"
