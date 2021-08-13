/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1997, 1998 Matthias Kalle Dalheimer <kalle@kde.org>
    SPDX-FileCopyrightText: 1999 Espen Sand <espen@kde.org>
    SPDX-FileCopyrightText: 2000-2004 Frerich Raabe <raabe@kde.org>
    SPDX-FileCopyrightText: 2003, 2004 Oswald Buddenhagen <ossi@kde.org>
    SPDX-FileCopyrightText: 2006 Thiago Macieira <thiago@kde.org>
    SPDX-FileCopyrightText: 2008 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ktoolinvocation.h"

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KSharedConfig>

#include "kservice.h"
#include <KConfig>
#include <KLocalizedString>
#include <KMacroExpander>
#include <KMessage>
#include <KShell>

#include <QDebug>
#include <QHash>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

static QStringList splitEmailAddressList(const QString &aStr)
{
    // This is a copy of KPIM::splitEmailAddrList().
    // Features:
    // - always ignores quoted characters
    // - ignores everything (including parentheses and commas)
    //   inside quoted strings
    // - supports nested comments
    // - ignores everything (including double quotes and commas)
    //   inside comments

    QStringList list;

    if (aStr.isEmpty()) {
        return list;
    }

    QString addr;
    int addrstart = 0;
    int commentlevel = 0;
    bool insidequote = false;

    for (int index = 0; index < aStr.length(); index++) {
        // the following conversion to latin1 is o.k. because
        // we can safely ignore all non-latin1 characters
        switch (aStr[index].toLatin1()) {
        case '"': // start or end of quoted string
            if (commentlevel == 0) {
                insidequote = !insidequote;
            }
            break;
        case '(': // start of comment
            if (!insidequote) {
                commentlevel++;
            }
            break;
        case ')': // end of comment
            if (!insidequote) {
                if (commentlevel > 0) {
                    commentlevel--;
                } else {
                    // qDebug() << "Error in address splitting: Unmatched ')'"
                    //          << endl;
                    return list;
                }
            }
            break;
        case '\\': // quoted character
            index++; // ignore the quoted character
            break;
        case ',':
            if (!insidequote && (commentlevel == 0)) {
                addr = aStr.mid(addrstart, index - addrstart);
                if (!addr.isEmpty()) {
                    list += addr.simplified();
                }
                addrstart = index + 1;
            }
            break;
        }
    }
    // append the last address to the list
    if (!insidequote && (commentlevel == 0)) {
        addr = aStr.mid(addrstart, aStr.length() - addrstart);
        if (!addr.isEmpty()) {
            list += addr.simplified();
        }
    }
    // else
    //  qDebug() << "Error in address splitting: "
    //            << "Unexpected end of address list"
    //            << endl;

    return list;
}

void KToolInvocation::invokeMailer(const QString &_to,
                                   const QString &_cc,
                                   const QString &_bcc,
                                   const QString &subject,
                                   const QString &body,
                                   const QString & /*messageFile TODO*/,
                                   const QStringList &attachURLs,
                                   const QByteArray &startup_id)
{
    if (!isMainThreadActive()) {
        return;
    }
    KService::Ptr emailClient = KApplicationTrader::preferredService(QStringLiteral("x-scheme-handler/mailto"));
    auto command = emailClient->exec();

    QString to;
    QString cc;
    QString bcc;
    if (emailClient->storageId() == QStringLiteral("org.kde.kmail2.desktop")) {
        command = QStringLiteral("kmail --composer -s %s -c %c -b %b --body %B --attach %A -- %t");
        if (!_to.isEmpty()) {
            QUrl url;
            url.setScheme(QStringLiteral("mailto"));
            url.setPath(_to);
            to = QString::fromLatin1(url.toEncoded());
        }
        if (!_cc.isEmpty()) {
            QUrl url;
            url.setScheme(QStringLiteral("mailto"));
            url.setPath(_cc);
            cc = QString::fromLatin1(url.toEncoded());
        }
        if (!_bcc.isEmpty()) {
            QUrl url;
            url.setScheme(QStringLiteral("mailto"));
            url.setPath(_bcc);
            bcc = QString::fromLatin1(url.toEncoded());
        }
    } else {
        to = _to;
        cc = _cc;
        bcc = _bcc;
        if (!command.contains(QLatin1Char('%'))) {
            command += QLatin1String(" %u");
        }
    }

    if (emailClient->terminal()) {
        KConfigGroup confGroup(KSharedConfig::openConfig(), "General");
        QString preferredTerminal = confGroup.readPathEntry("TerminalApplication", QStringLiteral("konsole"));
        command = preferredTerminal + QLatin1String(" -e ") + command;
    }

    QStringList cmdTokens = KShell::splitArgs(command);
    QString cmd = cmdTokens.takeFirst();

    QUrl url;
    QUrlQuery query;
    if (!to.isEmpty()) {
        QStringList tos = splitEmailAddressList(to);
        url.setPath(tos.first());
        tos.erase(tos.begin());
        for (QStringList::ConstIterator it = tos.constBegin(); it != tos.constEnd(); ++it) {
            query.addQueryItem(QStringLiteral("to"), *it);
        }
    }
    const QStringList ccs = splitEmailAddressList(cc);
    for (QStringList::ConstIterator it = ccs.constBegin(); it != ccs.constEnd(); ++it) {
        query.addQueryItem(QStringLiteral("cc"), *it);
    }
    const QStringList bccs = splitEmailAddressList(bcc);
    for (QStringList::ConstIterator it = bccs.constBegin(); it != bccs.constEnd(); ++it) {
        query.addQueryItem(QStringLiteral("bcc"), *it);
    }
    for (QStringList::ConstIterator it = attachURLs.constBegin(); it != attachURLs.constEnd(); ++it) {
        query.addQueryItem(QStringLiteral("attach"), *it);
    }
    if (!subject.isEmpty()) {
        query.addQueryItem(QStringLiteral("subject"), subject);
    }
    if (!body.isEmpty()) {
        query.addQueryItem(QStringLiteral("body"), body);
    }

    url.setQuery(query);

    if (!(to.isEmpty() && (!url.hasQuery()))) {
        url.setScheme(QStringLiteral("mailto"));
    }

    QHash<QChar, QString> keyMap;
    keyMap.insert(QLatin1Char('t'), to);
    keyMap.insert(QLatin1Char('s'), subject);
    keyMap.insert(QLatin1Char('c'), cc);
    keyMap.insert(QLatin1Char('b'), bcc);
    keyMap.insert(QLatin1Char('B'), body);
    keyMap.insert(QLatin1Char('u'), url.toString());

    QString attachlist = attachURLs.join(QLatin1Char(','));
    attachlist.prepend(QLatin1Char('\''));
    attachlist.append(QLatin1Char('\''));
    keyMap.insert(QLatin1Char('A'), attachlist);
    for (int i = 0; i < cmdTokens.count(); ++i) {
        if (cmdTokens.at(i) == QLatin1String("%A")) {
            if (attachURLs.isEmpty()) {
                cmdTokens.removeAt(i);
            } else {
                const QString previousStr = cmdTokens.at(i - 1);
                cmdTokens.removeAt(i);
                const int currentPos = i;
                for (const QString &attachUrl : attachURLs) {
                    cmdTokens.insert(currentPos, previousStr);
                    cmdTokens.insert(currentPos, attachUrl);
                    i += 2;
                }
            }
        } else {
            const QString str = KMacroExpander::expandMacros(cmdTokens.at(i), keyMap);
            cmdTokens[i] = str;
        }
    }
    QString error;
    // TODO this should check if cmd has a .desktop file, and use data from it, together
    // with sending more ASN data
    if (kdeinitExec(cmd, cmdTokens, &error, nullptr, startup_id)) {
        KMessage::message(KMessage::Error, //
                          i18n("Could not launch the mail client:\n\n%1", error),
                          i18n("Could not launch Mail Client"));
    }
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
void KToolInvocation::invokeBrowser(const QString &url, const QByteArray &startup_id)
{
    if (!isMainThreadActive()) {
        return;
    }

    QStringList args;
    args << url;
    QString error;

    // This method should launch a webbrowser, preferably without doing a MIME type
    // check first, like KRun (i.e. kde-open) would do.

    // In a KDE session, honour BrowserApplication if set, otherwise use preferred app for text/html if any,
    // otherwise xdg-open, otherwise kde-open (which does a MIME type check first though).

    // Outside KDE, call xdg-open if present, otherwise fallback to the above logic.

    QString exe; // the binary we are going to launch.

    const QString xdg_open = QStandardPaths::findExecutable(QStringLiteral("xdg-open"));
    if (qEnvironmentVariableIsEmpty("KDE_FULL_SESSION")) {
        exe = xdg_open;
    }

    if (exe.isEmpty()) {
        // We're in a KDE session (or there's no xdg-open installed)
        KConfigGroup config(KSharedConfig::openConfig(), "General");
        const QString browserApp = config.readPathEntry("BrowserApplication", QString());
        if (!browserApp.isEmpty()) {
            exe = browserApp;
            if (exe.startsWith(QLatin1Char('!'))) {
                exe.remove(0, 1); // Literal command
                QStringList cmdTokens = KShell::splitArgs(exe);
                exe = cmdTokens.takeFirst();
                args = cmdTokens + args;
            } else {
                // desktop file ID
                KService::Ptr service = KService::serviceByStorageId(exe);
                if (service) {
                    // qDebug() << "Starting service" << service->entryPath();
                    if (startServiceByDesktopPath(service->entryPath(), args, &error, nullptr, nullptr, startup_id)) {
                        KMessage::message(KMessage::Error,
                                          // TODO: i18n("Could not launch %1:\n\n%2", exe, error),
                                          i18n("Could not launch the browser:\n\n%1", error),
                                          i18n("Could not launch Browser"));
                    }
                    return;
                }
            }
        } else {
            const KService::Ptr htmlApp = KApplicationTrader::preferredService(QStringLiteral("text/html"));
            if (htmlApp) {
                // WORKAROUND: For bugs 264562 and 265474:
                // In order to correctly handle non-HTML urls we change the service
                // desktop file name to "kfmclient.desktop" whenever the above query
                // returns "kfmclient_html.desktop".Otherwise, the hard coded mime-type
                // "text/html" mime-type parameter in the kfmclient_html will cause all
                // URLs to be treated as if they are HTML page.
                QString entryPath = htmlApp->entryPath();
                if (entryPath.endsWith(QLatin1String("kfmclient_html.desktop"))) {
                    entryPath.remove(entryPath.length() - 13, 5);
                }
                QString error;
                int pid = 0;
                int err = startServiceByDesktopPath(entryPath, url, &error, nullptr, &pid, startup_id);
                if (err != 0) {
                    KMessage::message(KMessage::Error,
                                      // TODO: i18n("Could not launch %1:\n\n%2", htmlApp->exec(), error),
                                      i18n("Could not launch the browser:\n\n%1", error),
                                      i18n("Could not launch Browser"));
                } else { // success
                    return;
                }
            } else {
                exe = xdg_open;
            }
        }
    }

    if (exe.isEmpty()) {
        exe = QStringLiteral("kde-open"); // it's from kdebase-runtime, it has to be there.
    }

    // qDebug() << "Using" << exe << "to open" << url;
    if (kdeinitExec(exe, args, &error, nullptr, startup_id)) {
        KMessage::message(KMessage::Error,
                          // TODO: i18n("Could not launch %1:\n\n%2", exe, error),
                          i18n("Could not launch the browser:\n\n%1", error),
                          i18n("Could not launch Browser"));
    }
}
#endif

void KToolInvocation::invokeTerminal(const QString &command, const QStringList &envs, const QString &workdir, const QByteArray &startup_id)
{
    if (!isMainThreadActive()) {
        return;
    }

    const KService::Ptr terminal = terminalApplication(command, workdir);
    if (!terminal) {
        KMessage::message(KMessage::Error, i18n("Unable to determine the default terminal"));
        return;
    }

    QStringList cmdTokens = KShell::splitArgs(terminal->exec());
    const QString cmd = cmdTokens.takeFirst();

    QString error;
    // clang-format off
    if (self()->startServiceInternal("kdeinit_exec_with_workdir",
                                     cmd, cmdTokens, &error, nullptr, nullptr, startup_id, false, workdir, envs)) {
        KMessage::message(KMessage::Error,
                          i18n("Could not launch the terminal client:\n\n%1", error),
                          i18n("Could not launch Terminal Client"));
    }
    // clang-format on
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79)
void KToolInvocation::invokeTerminal(const QString &command, const QString &workdir, const QByteArray &startup_id)
{
    invokeTerminal(command, {}, workdir, startup_id);
}
#endif

KServicePtr KToolInvocation::terminalApplication(const QString &command, const QString &workingDir)
{
    const KConfigGroup confGroup(KSharedConfig::openConfig(), "General");
    const QString terminalService = confGroup.readEntry("TerminalService");
    const QString terminalExec = confGroup.readEntry("TerminalApplication");
    KServicePtr ptr;
    if (!terminalService.isEmpty()) {
        ptr = KService::serviceByStorageId(terminalService);
    } else if (!terminalExec.isEmpty()) {
        ptr = new KService(QStringLiteral("terminal"), terminalExec, QStringLiteral("utilities-terminal"));
    }
    if (!ptr) {
        ptr = KService::serviceByStorageId(QStringLiteral("org.kde.konsole"));
    }
    if (!ptr) {
        return KServicePtr();
    }
    QString exec = ptr->exec();
    if (!command.isEmpty()) {
        if (exec == QLatin1String("konsole")) {
            exec += QLatin1String(" --noclose");
        } else if (exec == QLatin1String("xterm")) {
            exec += QLatin1String(" -hold");
        }
        exec += QLatin1String(" -e ") + command;
    }
    if (ptr->exec() == QLatin1String("konsole") && !workingDir.isEmpty()) {
        exec += QStringLiteral(" --workdir %1").arg(KShell::quoteArg(workingDir));
    }
    ptr->setExec(exec);
    if (!workingDir.isEmpty()) {
        ptr->setWorkingDirectory(workingDir);
    }
    return ptr;
}
