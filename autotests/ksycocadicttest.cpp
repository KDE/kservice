/* This file is part of the KDE project
   Copyright (C) 2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <ksycoca.h>
#include <QBuffer>
#include <QtTest>
#include <QDebug>
#include <kservicetype.h>
#include <kdesktopfile.h>
#include <kconfiggroup.h>
#include <ksycocadict_p.h>

extern KSERVICE_EXPORT bool kservice_require_kded;

class KSycocaDictTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::enableTestMode(true);
        kservice_require_kded = false;

        // dicttestplugintype: a servicetype
        bool mustUpdateKSycoca = false;
        if (!KSycoca::isAvailable() || !KServiceType::serviceType("DictTestPluginType")) {
            mustUpdateKSycoca = true;
        }
        const QString dictTestPluginType = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservicetypes5/") + "dicttestplugintype.desktop";
        if (!QFile::exists(dictTestPluginType)) {
            mustUpdateKSycoca = true;
            KDesktopFile file(dictTestPluginType);
            KConfigGroup group = file.desktopGroup();
            group.writeEntry("Comment", "Fake Text Plugin");
            group.writeEntry("Type", "ServiceType");
            group.writeEntry("X-KDE-ServiceType", "DictTestPluginType");
            file.group("PropertyDef::X-KDE-Version").writeEntry("Type", "double"); // like in ktexteditorplugin.desktop
            qDebug() << "Just created" << dictTestPluginType;
        }
        if (mustUpdateKSycoca) {
            runKBuildSycoca();
        }

    }
    void testStandardDict();
    //void testExtensionDict();
private:
    void add(KSycocaDict &dict, const QString &key, const QString &name)
    {
        KServiceType::Ptr ptr = KServiceType::serviceType(name);
        if (!ptr) {
            qWarning() << "serviceType not found" << name;
        }
        dict.add(key, KSycocaEntry::Ptr(ptr));
    }
    static void runKBuildSycoca();
};

QTEST_MAIN(KSycocaDictTest)

void KSycocaDictTest::runKBuildSycoca()
{
    QSignalSpy spy(KSycoca::self(), SIGNAL(databaseChanged(QStringList)));
    QProcess proc;
    const QString kbuildsycoca = QStandardPaths::findExecutable(KBUILDSYCOCA_EXENAME);
    QVERIFY(!kbuildsycoca.isEmpty());
    QStringList args;
    args << "--testmode";
    proc.setProcessChannelMode(QProcess::MergedChannels); // silence kbuildsycoca output
    proc.start(kbuildsycoca, args);

    proc.waitForFinished();
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);

    qDebug() << "waiting for signal";
    QVERIFY(spy.wait(10000));
    qDebug() << "got signal";

}

// Standard use of KSycocaDict: mapping entry name to entry
void KSycocaDictTest::testStandardDict()
{
    QVERIFY(KSycoca::isAvailable());

    QStringList serviceTypes;
    serviceTypes << "DictTestPluginType"
                 << "KUriFilter/Plugin"
                 << "KDataTool"
                 << "KCModule"
                 << "KScan/KScanDialog"
                 << "Browser/View"
                 << "Plasma/Applet"
                 << "Plasma/Runner";

    // Skip servicetypes that are not installed
    QMutableListIterator<QString> it(serviceTypes);
    while (it.hasNext()) {
        if (!KServiceType::serviceType(it.next())) {
            it.remove();
        }
    }
    qDebug() << serviceTypes;

    QBENCHMARK {
        QByteArray buffer;
        {
            KSycocaDict dict;
            foreach (const QString &str, serviceTypes)
            {
                add(dict, str, str);
            }
            dict.remove("DictTestPluginType"); // just to test remove
            add(dict, "DictTestPluginType", "DictTestPluginType");
            QCOMPARE(int(dict.count()), serviceTypes.count());
            QDataStream saveStream(&buffer, QIODevice::WriteOnly);
            dict.save(saveStream);
        }

        QDataStream stream(buffer);
        KSycocaDict loadingDict(&stream, 0);
        int offset = loadingDict.find_string("DictTestPluginType");
        QVERIFY(offset > 0);
        QCOMPARE(offset, KServiceType::serviceType("DictTestPluginType")->offset());
        foreach (const QString &str, serviceTypes)
        {
            int offset = loadingDict.find_string(str);
            QVERIFY(offset > 0);
            QCOMPARE(offset, KServiceType::serviceType(str)->offset());
        }
        offset = loadingDict.find_string("doesnotexist");
        // TODO QCOMPARE(offset, 0); // could be non 0 according to the docs, too; if non 0, we should check that the pointed mimetype doesn't have this name.
    }
}

#include "ksycocadicttest.moc"
