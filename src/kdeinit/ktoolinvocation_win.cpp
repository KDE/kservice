/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2004-2008 Jarosław Staniek <staniek@kde.org>
    SPDX-FileCopyrightText: 2006 Ralf Habacker <ralf.habacker@freenet.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "ktoolinvocation.h"

#include <KMessage>
#include <KLocalizedString>

#include <QUrl>
#include <QUrlQuery>
#include <QProcess>
#include <QCoreApplication>
#include <QHash>
#include <QDBusConnection>

#include "windows.h"
#include "shellapi.h"

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

void KToolInvocation::invokeMailer(const QString &_to, const QString &_cc, const QString &_bcc,
                                   const QString &subject, const QString &body,
                                   const QString & /*messageFile TODO*/, const QStringList &attachURLs,
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

void KToolInvocation::invokeTerminal(const QString &command,
                                     const QStringList &envs,
                                     const QString &workdir,
                                     const QByteArray &startup_id)
{
    //TODO
}

void KToolInvocation::invokeTerminal(const QString &command, const QString &workdir, const QByteArray &startup_id)
{
    invokeTerminal(command, QStringList(), workdir, startup_id);
}
