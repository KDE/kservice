/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2004-2008 Jarosław Staniek <staniek@kde.org>
    SPDX-FileCopyrightText: 2006 Ralf Habacker <ralf.habacker@freenet.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "ktoolinvocation.h"

#include "kservice.h"

#include <KLocalizedString>
#include <KMessage>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QHash>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

#include "windows.h"

#include "shellapi.h" // Must be included after "windows.h"

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
void KToolInvocation::invokeBrowser(const QString &url, const QByteArray &startup_id)
{
#ifndef _WIN32_WCE
    QString sOpen = QString::fromLatin1("open");
    ShellExecuteW(0, (LPCWSTR)sOpen.utf16(), (LPCWSTR)url.utf16(), 0, 0, SW_NORMAL);
#else
    SHELLEXECUTEINFO cShellExecuteInfo = {0};
    cShellExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    cShellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    cShellExecuteInfo.hwnd = NULL;
    cShellExecuteInfo.lpVerb = L"Open";
    cShellExecuteInfo.lpFile = (LPCWSTR)url.utf16();
    cShellExecuteInfo.nShow = SW_SHOWNORMAL;
    ShellExecuteEx(&cShellExecuteInfo);
#endif
}
#endif

void KToolInvocation::invokeMailer(const QString &_to,
                                   const QString &_cc,
                                   const QString &_bcc,
                                   const QString &subject,
                                   const QString &body,
                                   const QString & /*messageFile TODO*/,
                                   const QStringList &attachURLs,
                                   const QByteArray &startup_id)
{
    QUrl url(QLatin1String("mailto:") + _to);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("subject"), subject);
    query.addQueryItem(QStringLiteral("cc"), _cc);
    query.addQueryItem(QStringLiteral("bcc"), _bcc);
    query.addQueryItem(QStringLiteral("body"), body);
    for (const QString &attachURL : attachURLs) {
        query.addQueryItem(QStringLiteral("attach"), attachURL);
    }
    url.setQuery(query);

#ifndef _WIN32_WCE
    QString sOpen = QLatin1String("open");
    ShellExecuteW(0, (LPCWSTR)sOpen.utf16(), (LPCWSTR)url.url().utf16(), 0, 0, SW_NORMAL);
#else
    SHELLEXECUTEINFO cShellExecuteInfo = {0};
    cShellExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    cShellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    cShellExecuteInfo.hwnd = NULL;
    cShellExecuteInfo.lpVerb = L"Open";
    cShellExecuteInfo.lpFile = (LPCWSTR)url.url().utf16();
    cShellExecuteInfo.nShow = SW_SHOWNORMAL;
    ShellExecuteEx(&cShellExecuteInfo);
#endif
}

void KToolInvocation::invokeTerminal(const QString &command, const QStringList &envs, const QString &workdir, const QByteArray &startup_id)
{
    const QString windowsTerminal = QStringLiteral("wt.exe");
    const QString pwsh = QStringLiteral("pwsh.exe");
    const QString powershell = QStringLiteral("powershell.exe"); // Powershell is used as fallback
    const bool hasWindowsTerminal = !QStandardPaths::findExecutable(windowsTerminal).isEmpty();
    const bool hasPwsh = !QStandardPaths::findExecutable(pwsh).isEmpty();

    QProcess process;
    QStringList args;
    process.setWorkingDirectory(workdir);

    if (hasWindowsTerminal) {
        process.setProgram(windowsTerminal);
        if (!workdir.isEmpty()) {
            args << QStringLiteral("--startingDirectory") << workdir;
        }
        if (!command.isEmpty()) {
            // Command and NoExit flag will be added later
            args << (hasPwsh ? pwsh : powershell);
        }
    } else {
        process.setProgram(hasPwsh ? pwsh : powershell);
    }
    if (!command.isEmpty()) {
        args << QStringLiteral("-NoExit") << QStringLiteral("-Command") << command;
    }
    process.setArguments(args);

    if (!envs.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (const QString &envVar : envs) {
            const int splitIndex = envVar.indexOf(QLatin1Char('='));
            env.insert(envVar.left(splitIndex), envVar.mid(splitIndex + 1));
        }
        process.setProcessEnvironment(env);
    }
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NEW_CONSOLE;
        args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
    });
    process.startDetached();
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79)
void KToolInvocation::invokeTerminal(const QString &command, const QString &workdir, const QByteArray &startup_id)
{
    invokeTerminal(command, QStringList(), workdir, startup_id);
}
#endif

KServicePtr KToolInvocation::terminalApplication(const QString &command, const QString &workingDir)
{
    return KServicePtr();
}
