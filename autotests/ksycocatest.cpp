/* This file is part of the KDE project
   Copyright (C) 2015 David Faure <faure@kde.org>

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
#include <QTest>
#include <QDebug>
#include <kservicetype.h>
#include <kdesktopfile.h>
#include <kconfiggroup.h>
#include <QSignalSpy>
#include <QProcess>
#include <kservice.h>

class KSycocaTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::enableTestMode(true);

        QVERIFY(m_tempDir.isValid());

        // we don't need the services dir -> ensure there isn't one, so we can check allResourceDirs below.
        QDir(servicesDir()).removeRecursively();

        runKBuildSycoca(QProcessEnvironment::systemEnvironment());
    }
    void testAllResourceDirs();
    void testOtherAppDir();

private:
    QString servicesDir() { return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kservices5"; }
    QString serviceTypesDir() { return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kservicetypes5"; }

    static void runKBuildSycoca(const QProcessEnvironment &environment);

    QTemporaryDir m_tempDir;
};

QTEST_MAIN(KSycocaTest)

void KSycocaTest::runKBuildSycoca(const QProcessEnvironment &environment)
{
    QSignalSpy spy(KSycoca::self(), SIGNAL(databaseChanged(QStringList)));
    QProcess proc;
    const QString kbuildsycoca = QStandardPaths::findExecutable(KBUILDSYCOCA_EXENAME);
    QVERIFY(!kbuildsycoca.isEmpty());
    QStringList args;
    args << "--testmode";
    proc.setProcessChannelMode(QProcess::MergedChannels); // silence kbuildsycoca output
    proc.start(kbuildsycoca, args);
    proc.setProcessEnvironment(environment);

    proc.waitForFinished();
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);

    qDebug() << "waiting for signal";
    QVERIFY(spy.wait(10000));
    qDebug() << "got signal";
}

void KSycocaTest::testAllResourceDirs()
{
    // Dirs that exist and dirs that don't exist, should both in allResourceDirs().
    const QStringList dirs = KSycoca::self()->allResourceDirs();
    QVERIFY2(dirs.contains(servicesDir()), qPrintable(dirs.join(',')));
    QVERIFY2(dirs.contains(serviceTypesDir()), qPrintable(dirs.join(',')));
}

// taken from tst_qstandardpaths
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_BLACKBERRY) && !defined(Q_OS_ANDROID)
#define Q_XDG_PLATFORM
#endif

void KSycocaTest::testOtherAppDir()
{
#ifndef Q_XDG_PLATFORM
    QSKIP("This test requires XDG_DATA_DIRS");
#endif

    const QString dataDir = m_tempDir.path();
    qputenv("XDG_DATA_DIRS", QFile::encodeName(dataDir));
    QCOMPARE(dataDir, QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).last());
    QVERIFY(!KService::serviceByDesktopPath("test_app_other.desktop"));

    const QString appDir = dataDir + "/applications";

    // test_app_other: live in a different application directory
    const QString testAppOther = appDir + "/test_app_other.desktop";
    KDesktopFile file(testAppOther);
    KConfigGroup group = file.desktopGroup();
    group.writeEntry("Type", "Application");
    group.writeEntry("Exec", "kded5");
    group.writeEntry("Name", "Test App Other");
    qDebug() << "Just created" << testAppOther;
    file.sync();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("XDG_DATA_DIRS", dataDir);
    runKBuildSycoca(env);

#if 0 // for debugging
    const KService::List lst = KService::allServices();
    QVERIFY(!lst.isEmpty());
    Q_FOREACH (const KService::Ptr &serv, lst) {
        qDebug() << serv->entryPath() << serv->storageId() /*<< serv->desktopEntryName()*/;
    }
#endif

    QVERIFY(KService::serviceByStorageId("test_app_other.desktop"));
}

#include "ksycocatest.moc"
