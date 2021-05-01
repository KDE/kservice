/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2005 Brad Hards <bradh@frogmouth.net>
    SPDX-FileCopyrightText: 2006 Thiago Macieira <thiago@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ktoolinvocation.h"
#ifdef QT_DBUS_LIB
#include "klauncher_iface.h"
#include <KDEInitInterface>
#endif
#include <KLocalizedString>

#include <QCoreApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>

#include <errno.h> // for EINVAL

class KToolInvocationSingleton
{
public:
    KToolInvocation instance;
};

Q_GLOBAL_STATIC(KToolInvocationSingleton, s_self)

KToolInvocation *KToolInvocation::self()
{
    return &s_self()->instance;
}

KToolInvocation::KToolInvocation()
    : QObject(nullptr)
    , d(nullptr)
{
}

KToolInvocation::~KToolInvocation()
{
}

static void printError(const QString &text, QString *error)
{
    if (error) {
        *error = text;
    } else {
        qWarning() << text;
    }
}

bool KToolInvocation::isMainThreadActive(QString *error)
{
    if (QCoreApplication::instance() && QCoreApplication::instance()->thread() != QThread::currentThread()) {
        printError(i18n("Function must be called from the main thread."), error);
        return false;
    }

    return true;
}

int KToolInvocation::startServiceInternal(const char *_function,
                                          const QString &_name,
                                          const QStringList &URLs,
                                          QString *error,
                                          QString *serviceName,
                                          int *pid,
                                          const QByteArray &startup_id,
                                          bool noWait,
                                          const QString &workdir,
                                          const QStringList &envs)
{
#ifdef QT_DBUS_LIB
    QString function = QLatin1String(_function);
    KToolInvocation::ensureKdeinitRunning();
    QDBusMessage msg =
        QDBusMessage::createMethodCall(QStringLiteral("org.kde.klauncher5"), QStringLiteral("/KLauncher"), QStringLiteral("org.kde.KLauncher"), function);
    msg << _name << URLs;
    if (function == QLatin1String("kdeinit_exec_with_workdir")) {
        msg << workdir;
    }
    // make sure there is id, so that user timestamp exists
    QByteArray s = startup_id;
    QStringList envCopy(envs);
    Q_EMIT kapplication_hook(envCopy, s);
    msg << envCopy;
    msg << QString::fromLatin1(s);
    if (!function.startsWith(QLatin1String("kdeinit_exec"))) {
        msg << noWait;
    }

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, INT_MAX);
    if (reply.type() != QDBusMessage::ReplyMessage) {
        QDBusReply<QString> replyObj(reply);
        if (replyObj.error().type() == QDBusError::NoReply) {
            printError(i18n("Error launching %1. Either KLauncher is not running anymore, or it failed to start the application.", _name), error);
        } else {
            const QString rpl = reply.arguments().count() > 0 ? reply.arguments().at(0).toString() : reply.errorMessage();
            printError(i18n("KLauncher could not be reached via D-Bus. Error when calling %1:\n%2\n", function, rpl), error);
        }
        // qDebug() << reply;
        return EINVAL;
    }

    if (noWait) {
        return 0;
    }

    Q_ASSERT(reply.arguments().count() == 4);
    if (serviceName) {
        *serviceName = reply.arguments().at(1).toString();
    }
    if (error) {
        *error = reply.arguments().at(2).toString();
    }
    if (pid) {
        *pid = reply.arguments().at(3).toInt();
    }
    return reply.arguments().at(0).toInt();
#else
    return ENOTSUP;
#endif
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
int KToolInvocation::startServiceByName(const QString &_name,
                                        const QString &URL,
                                        QString *error,
                                        QString *serviceName,
                                        int *pid,
                                        const QByteArray &startup_id,
                                        bool noWait)
{
    if (!isMainThreadActive(error)) {
        return EINVAL;
    }

    QStringList URLs;
    if (!URL.isEmpty()) {
        URLs.append(URL);
    }
    return self()->startServiceInternal("start_service_by_name", _name, URLs, error, serviceName, pid, startup_id, noWait);
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
int KToolInvocation::startServiceByName(const QString &_name,
                                        const QStringList &URLs,
                                        QString *error,
                                        QString *serviceName,
                                        int *pid,
                                        const QByteArray &startup_id,
                                        bool noWait)
{
    if (!isMainThreadActive(error)) {
        return EINVAL;
    }

    return self()->startServiceInternal("start_service_by_name", _name, URLs, error, serviceName, pid, startup_id, noWait);
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 82)
int KToolInvocation::startServiceByDesktopPath(const QString &_name,
                                               const QString &URL,
                                               QString *error,
                                               QString *serviceName,
                                               int *pid,
                                               const QByteArray &startup_id,
                                               bool noWait)
{
    if (!isMainThreadActive(error)) {
        return EINVAL;
    }

    QStringList URLs;
    if (!URL.isEmpty()) {
        URLs.append(URL);
    }
    return self()->startServiceInternal("start_service_by_desktop_path", _name, URLs, error, serviceName, pid, startup_id, noWait);
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 82)
int KToolInvocation::startServiceByDesktopPath(const QString &_name,
                                               const QStringList &URLs,
                                               QString *error,
                                               QString *serviceName,
                                               int *pid,
                                               const QByteArray &startup_id,
                                               bool noWait)
{
    if (!isMainThreadActive(error)) {
        return EINVAL;
    }

    return self()->startServiceInternal("start_service_by_desktop_path", _name, URLs, error, serviceName, pid, startup_id, noWait);
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 82)
int KToolInvocation::startServiceByDesktopName(const QString &_name,
                                               const QString &URL,
                                               QString *error,
                                               QString *serviceName,
                                               int *pid,
                                               const QByteArray &startup_id,
                                               bool noWait)
{
    if (!isMainThreadActive(error)) {
        return EINVAL;
    }

    QStringList URLs;
    if (!URL.isEmpty()) {
        URLs.append(URL);
    }
    return self()->startServiceInternal("start_service_by_desktop_name", _name, URLs, error, serviceName, pid, startup_id, noWait);
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 82)
int KToolInvocation::startServiceByDesktopName(const QString &_name,
                                               const QStringList &URLs,
                                               QString *error,
                                               QString *serviceName,
                                               int *pid,
                                               const QByteArray &startup_id,
                                               bool noWait)
{
    if (!isMainThreadActive(error)) {
        return EINVAL;
    }

    return self()->startServiceInternal("start_service_by_desktop_name", _name, URLs, error, serviceName, pid, startup_id, noWait);
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 83)
int KToolInvocation::kdeinitExec(const QString &name, const QStringList &args, QString *error, int *pid, const QByteArray &startup_id)
{
    if (!isMainThreadActive(error)) {
        return EINVAL;
    }

    return self()->startServiceInternal("kdeinit_exec", name, args, error, nullptr, pid, startup_id, false);
}
#endif

int KToolInvocation::kdeinitExecWait(const QString &name, const QStringList &args, QString *error, int *pid, const QByteArray &startup_id)
{
    if (!isMainThreadActive(error)) {
        return EINVAL;
    }

    return self()->startServiceInternal("kdeinit_exec_wait", name, args, error, nullptr, pid, startup_id, false);
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
void KToolInvocation::invokeMailer(const QString &address, const QString &subject, const QByteArray &startup_id)
{
    if (!isMainThreadActive()) {
        return;
    }

    invokeMailer(address, QString(), QString(), subject, QString(), QString(), QStringList(), startup_id);
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
void KToolInvocation::invokeMailer(const QUrl &mailtoURL, const QByteArray &startup_id, bool allowAttachments)
{
    if (!isMainThreadActive()) {
        return;
    }

    QString address = mailtoURL.path();
    QString subject;
    QString cc;
    QString bcc;
    QString body;

    QList<QPair<QString, QString>> queryItems = QUrlQuery(mailtoURL).queryItems();
    const QChar comma = QChar::fromLatin1(',');
    QStringList attachURLs;
    for (int i = 0; i < queryItems.count(); ++i) {
        const QString q = queryItems.at(i).first.toLower();
        const QString value = queryItems.at(i).second;
        if (q == QLatin1String("subject")) {
            subject = value;
        } else if (q == QLatin1String("cc")) {
            cc = cc.isEmpty() ? value : cc + comma + value;
        } else if (q == QLatin1String("bcc")) {
            bcc = bcc.isEmpty() ? value : bcc + comma + value;
        } else if (q == QLatin1String("body")) {
            body = value;
        } else if (allowAttachments && q == QLatin1String("attach")) {
            attachURLs.push_back(value);
        } else if (allowAttachments && q == QLatin1String("attachment")) {
            attachURLs.push_back(value);
        } else if (q == QLatin1String("to")) {
            address = address.isEmpty() ? value : address + comma + value;
        }
    }

    invokeMailer(address, cc, bcc, subject, body, QString(), attachURLs, startup_id);
}
#endif

void KToolInvocation::ensureKdeinitRunning()
{
#ifdef QT_DBUS_LIB
    KDEInitInterface::ensureKdeinitRunning();
#endif
}
