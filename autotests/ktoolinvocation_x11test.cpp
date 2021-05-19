/*
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KConfigGroup>
#include <KService>
#include <KSharedConfig>
#include <QTest>
#include <ktoolinvocation.h>

class KToolInvocationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testTerminalEntryParsing();
    void testLegacyEntryParsing();
    void testTerminalEntryParsingWithParameters();
    void testNoKonsole();

private:
    KConfigGroup general;
};

void KToolInvocationTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    general = KSharedConfig::openConfig()->group("General");
}

void KToolInvocationTest::testTerminalEntryParsing()
{
    general.writeEntry("TerminalService", QFINDTESTDATA("org.kde.konsole.desktop"));
    general.sync();

    const KService::Ptr ptr = KToolInvocation::terminalApplication();
    QVERIFY(ptr->isValid());
    QCOMPARE(ptr->exec(), QStringLiteral("konsole"));
    QCOMPARE(ptr->workingDirectory(), QString());
}

void KToolInvocationTest::testLegacyEntryParsing()
{
    general.deleteGroup();
    general.writeEntry("TerminalApplication", "/bin/true");
    general.sync();

    const KService::Ptr ptr = KToolInvocation::terminalApplication();
    QVERIFY(ptr->isValid());
    QCOMPARE(ptr->exec(), QStringLiteral("/bin/true"));
    QCOMPARE(ptr->workingDirectory(), QString());
}

void KToolInvocationTest::testTerminalEntryParsingWithParameters()
{
    general.writeEntry("TerminalService", QFINDTESTDATA("org.kde.konsole.desktop"));
    general.sync();

    const KService::Ptr ptr = KToolInvocation::terminalApplication(QStringLiteral("/bin/true"), QDir::homePath());
    QVERIFY(ptr->isValid());
    QCOMPARE(ptr->exec(), QStringLiteral("konsole --noclose -e /bin/true --workdir %1").arg(QDir::homePath()));
    QCOMPARE(ptr->workingDirectory(), QDir::homePath());
}

void KToolInvocationTest::testNoKonsole()
{
    general.deleteGroup();
    general.sync();

    const KService::Ptr ptr = KToolInvocation::terminalApplication();
    QVERIFY(!ptr);
}

QTEST_MAIN(KToolInvocationTest)

#include "ktoolinvocation_x11test.moc"
