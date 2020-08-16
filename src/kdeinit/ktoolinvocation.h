/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1997-1999 Matthias Kalle Dalheimer <kalle@kde.org>
    SPDX-FileCopyrightText: 1997-2000 Matthias Ettrich <ettrich@troll.no>
    SPDX-FileCopyrightText: 1998-2005 Stephan Kulow <coolo@kde.org>
    SPDX-FileCopyrightText: 1999-2004 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2001-2005 Lubos Lunak <l.lunak@kde.org>
    SPDX-FileCopyrightText: 2008 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KTOOLINVOCATION_H
#define _KTOOLINVOCATION_H

#include <kservice_export.h>

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 89)
#include <QByteArray>
#include <QExplicitlySharedDataPointer>
#include <QObject>
#include <QStringList>

class QUrl;
class KToolInvocationPrivate;
class KService;
typedef QExplicitlySharedDataPointer<KService> KServicePtr;

/**
 * @class KToolInvocation ktoolinvocation.h <KToolInvocation>
 *
 * KToolInvocation: for starting other programs
 *
 * @section desktopfiles Desktop files for startServiceBy
 *
 * The way a service gets started depends on the 'X-DBUS-StartupType'
 * entry in the desktop file of the service:
 *
 * There are three possibilities:
 * @li X-DBUS-StartupType=None (default)
 *    Always start a new service,
 *    don't wait till the service registers with D-Bus.
 * @li X-DBUS-StartupType=Multi
 *    Always start a new service,
 *    wait until the service has registered with D-Bus.
 * @li X-DBUS-StartupType=Unique
 *    Only start the service if it isn't already running,
 *    wait until the service has registered with D-Bus.
 * The .desktop file can specify the name that the application will use when registering
 * using X-DBUS-ServiceName=org.domain.mykapp. Otherwise org.kde.binaryname is assumed.
 *
 * @section thread Multi-threading
 *
 * The static members (apart from self()), have to be called from the QApplication main thread.
 * Calls to members are only allowed if there is a Q(Core)Application object created
 * If you call the members with signal/slot connections across threads, you can't use the return values
 * If a function is called from the wrong thread and it has a return value -1 is returned
 * Investigate if this is really needed or if D-Bus is threadsafe anyway
 *
 * For more details see <a
 * href="http://techbase.kde.org/Development/Architecture/KDE4/Starting_Other_Programs#KToolInvocation::startServiceByDesktopPath">techbase</a>.
 *
 */
class KSERVICE_EXPORT KToolInvocation : public QObject
{
    Q_OBJECT
private:
    KToolInvocation();

public:
    // @internal
    ~KToolInvocation() override;
    static KToolInvocation *self();

public Q_SLOTS:

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 0)
    /**
     * Convenience method; invokes the standard email application.
     *
     * @param address The destination address
     * @param subject Subject string. Can be QString().
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     *
     * @deprecated since 5.0, use QDesktopServices::openUrl(mailtoURL),
     * using QUrl::setPath(address) and a query item of "subject" for the subject.
     */
    KSERVICE_DEPRECATED_VERSION(5, 0, "Use QDesktopServices::openUrl(mailtoURL), using QUrl::setPath(address) and a query item of \"subject\" for the subject")
    static void invokeMailer(const QString &address, const QString &subject, const QByteArray &startup_id = QByteArray());
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 0)
    /**
     * Invokes the standard email application.
     *
     * @param mailtoURL A mailto URL.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @param allowAttachments whether attachments specified in mailtoURL should be honoured.
     *           The default is false; do not honor requests for attachments.
     * @deprecated since 5.0, use QDesktopServices::openUrl(mailtoURL)
     */
    KSERVICE_DEPRECATED_VERSION(5, 0, "Use QDesktopServices::openUrl(const QUrl&)")
    static void invokeMailer(const QUrl &mailtoURL, const QByteArray &startup_id = QByteArray(), bool allowAttachments = false);
#endif

    /**
     * Convenience method; invokes the standard email application.
     *
     * All parameters are optional.
     *
     * @param to          The destination address.
     * @param cc          The Cc field
     * @param bcc         The Bcc field
     * @param subject     Subject string
     * @param body        A string containing the body of the mail (exclusive with messageFile)
     * @param messageFile A file (URL) containing the body of the mail (exclusive with body) - currently unsupported
     * @param attachURLs  List of URLs to be attached to the mail.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @deprecated Since 5.89, use KEMailClientLauncherJob instead
     */
    KSERVICE_DEPRECATED_VERSION(5, 89, "use KEMailClientLauncherJob instead")
    static void invokeMailer(const QString &to,
                             const QString &cc,
                             const QString &bcc,
                             const QString &subject,
                             const QString &body,
                             const QString &messageFile = QString(),
                             const QStringList &attachURLs = QStringList(),
                             const QByteArray &startup_id = QByteArray());

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 0)
    /**
     * Invokes the user's preferred browser.
     * Note that you should only do this when you know for sure that the browser can
     * handle the URL (i.e. its MIME type). In doubt, if the URL can point to an image
     * or anything else than HTML, prefer to use new KRun( url ).
     *
     * See also <a
     * href="http://techbase.kde.org/Development/Architecture/KDE4/Starting_Other_Programs#KToolInvocation::invokeBrowser>techbase</a>
     * for a discussion of invokeBrowser vs KRun.
     *
     * @param url The destination address
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @deprecated since 5.0, use QDesktopServices::openUrl(url)
     */
    KSERVICE_DEPRECATED_VERSION(5, 0, "Use QDesktopServices::openUrl(const QUrl&)")
    static void invokeBrowser(const QString &url, const QByteArray &startup_id = QByteArray());
#endif

    /**
     * Invokes the standard terminal application.
     *
     * @param command the command to execute, can be empty.
     * @param envs ENV variables for the invoked terminal command
     * @param workdir the initial working directory, can be empty.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @since 5.73
     * @deprecated Since 5.89, use KTerminalLauncherJob instead
     */
    KSERVICE_DEPRECATED_VERSION(5, 89, "use KTerminalLauncherJob instead")
    static void invokeTerminal(const QString &command, const QStringList &envs, const QString &workdir = QString(), const QByteArray &startup_id = "");

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 79)
    /**
     *
     * Invokes the standard terminal application.
     *
     * @param command the command to execute, can be empty.
     * @param workdir the initial working directory, can be empty.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     *
     * @since 4.1
     * @deprecated since 5,79, use invokeTerminal(const QString &command, const QStringList &envs, const QString &workdir, const QByteArray &startup_id) instead
     */
    KSERVICE_DEPRECATED_VERSION(
        5,
        79,
        "Use invokeTerminal(const QString &command, const QStringList &envs, const QString &workdir, const QByteArray &startup_id) instead")
    static void invokeTerminal(const QString &command, const QString &workdir = QString(), const QByteArray &startup_id = "");
#endif

    /**
     * Returns the configured default terminal application. This is compatible with
     * the old config format from the KCM in Plasma < 5.21.
     * @param command Command that should be executed in the terminal app
     * @param workingDir Working directory for the terminal app.
     * @return KServicePtr of terminal application.
     * @since 5.78
     * @deprecated Since 5.89, use KTerminalLauncherJob instead to launch the executable
     */
    KSERVICE_DEPRECATED_VERSION(5, 89, "use KTerminalLauncherJob instead to launch the executable")
    static KServicePtr terminalApplication(const QString &command = QString(), const QString &workingDir = QString());

public:
#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 0)
    /**
     * Starts a service based on the (translated) name of the service.
     * E.g. "Web Browser"
     *
     * @param _name the name of the service
     * @param URL if not empty this URL is passed to the service
     * @param error On failure, error contains a description of the error
     *         that occurred. If the pointer is 0, the argument will be
     *         ignored
     * @param serviceName On success, serviceName contains the D-Bus name
     *         under which this service is available. If empty, the service does
     *         not provide D-Bus services. If the pointer is 0 the argument
     *         will be ignored
     * @param pid On success, the process id of the new service will be written
     *        here. If the pointer is 0, the argument will be ignored.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @param noWait if set, the function does not wait till the service is running.
     * @return an error code indicating success (== 0) or failure (> 0).
     * @deprecated Since 5.0, use startServiceByDesktopName or startServiceByDesktopPath
     */
    KSERVICE_DEPRECATED_VERSION(5, 0, "Use KToolInvocation::startServiceByDesktopName(...) or KToolInvocation::startServiceByDesktopPath(...)")
    static int startServiceByName(const QString &_name,
                                  const QString &URL,
                                  QString *error = nullptr,
                                  QString *serviceName = nullptr,
                                  int *pid = nullptr,
                                  const QByteArray &startup_id = QByteArray(),
                                  bool noWait = false);
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 0)
    /**
     * Starts a service based on the (translated) name of the service.
     * E.g. "Web Browser"
     *
     * @param _name the name of the service
     * @param URLs if not empty these URLs will be passed to the service
     * @param error On failure, @p error contains a description of the error
     *         that occurred. If the pointer is 0, the argument will be
     *         ignored
     * @param serviceName On success, @p serviceName contains the D-Bus name
     *         under which this service is available. If empty, the service does
     *         not provide D-Bus services. If the pointer is 0 the argument
     *         will be ignored
     * @param pid On success, the process id of the new service will be written
     *        here. If the pointer is 0, the argument will be ignored.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @param noWait if set, the function does not wait till the service is running.
     * @return an error code indicating success (== 0) or failure (> 0).
     * @deprecated Since 5.0, use startServiceByDesktopName or startServiceByDesktopPath
     */
    KSERVICE_DEPRECATED_VERSION(5, 0, "Use KToolInvocation::startServiceByDesktopName(...) or KToolInvocation::startServiceByDesktopPath(...)")
    static int startServiceByName(const QString &_name,
                                  const QStringList &URLs = QStringList(),
                                  QString *error = nullptr,
                                  QString *serviceName = nullptr,
                                  int *pid = nullptr,
                                  const QByteArray &startup_id = QByteArray(),
                                  bool noWait = false);
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 82)
    /**
     * Starts a service based on the desktop path of the service.
     * E.g. "Applications/konqueror.desktop" or "/home/user/bla/myfile.desktop"
     *
     * @param _name the path of the desktop file
     * @param URL if not empty this URL is passed to the service
     * @param error On failure, @p error contains a description of the error
     *         that occurred. If the pointer is 0, the argument will be
     *         ignored
     * @param serviceName On success, @p serviceName contains the D-Bus name
     *         under which this service is available. If empty, the service does
     *         not provide D-Bus services. If the pointer is 0 the argument
     *         will be ignored
     * @param pid On success, the process id of the new service will be written
     *        here. If the pointer is 0, the argument will be ignored.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @param noWait if set, the function does not wait till the service is running.
     * @return an error code indicating success (== 0) or failure (> 0).
     *
     * @deprecated since 5.0 use QDBusConnectionInterface::startService("org.kde.serviceName"),
     *   to start a unique application in order to make D-Bus calls to it (after ensuring that
     *   it installs a D-Bus org.kde.serviceName.service file). Otherwise just use QProcess,
     *   KIO::CommandLauncherJob or KIO::ApplicationLauncherJob.
     */
    KSERVICE_DEPRECATED_VERSION_BELATED(5, 82, 5, 0, "See API documentation")
    static int startServiceByDesktopPath(const QString &_name,
                                         const QString &URL,
                                         QString *error = nullptr,
                                         QString *serviceName = nullptr,
                                         int *pid = nullptr,
                                         const QByteArray &startup_id = QByteArray(),
                                         bool noWait = false);
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 82)
    /**
     * Starts a service based on the desktop path of the service.
     * E.g. "Applications/konqueror.desktop" or "/home/user/bla/myfile.desktop"
     *
     * @param _name the path of the desktop file
     * @param URLs if not empty these URLs will be passed to the service
     * @param error On failure, @p error contains a description of the error
     *         that occurred. If the pointer is 0, the argument will be
     *         ignored   * @param serviceName On success, @p serviceName contains the D-Bus name
     *         under which this service is available. If empty, the service does
     *         not provide D-Bus services. If the pointer is 0 the argument
     *         will be ignored
     * @param pid On success, the process id of the new service will be written
     *        here. If the pointer is 0, the argument will be ignored.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @param noWait if set, the function does not wait till the service is running.
     * @return an error code indicating success (== 0) or failure (> 0).
     * @deprecated since 5.0 use QDBusConnectionInterface::startService("org.kde.serviceName"),
     *   to start a unique application in order to make D-Bus calls to it (after ensuring that
     *   it installs a D-Bus org.kde.serviceName.service file). Otherwise just use QProcess,
     *   KIO::CommandLauncherJob or KIO::ApplicationLauncherJob.
     */
    KSERVICE_DEPRECATED_VERSION_BELATED(5, 82, 5, 0, "See API documentation")
    static int startServiceByDesktopPath(const QString &_name,
                                         const QStringList &URLs = QStringList(),
                                         QString *error = nullptr,
                                         QString *serviceName = nullptr,
                                         int *pid = nullptr,
                                         const QByteArray &startup_id = QByteArray(),
                                         bool noWait = false);
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 82)
    /**
     * Starts a service based on the desktop name of the service.
     * E.g. "konqueror"
     *
     * @param _name the desktop name of the service
     * @param URL if not empty this URL is passed to the service
     * @param error On failure, @p error contains a description of the error
     *         that occurred. If the pointer is 0, the argument will be
     *         ignored
     * @param serviceName On success, @p serviceName contains the D-Bus service name
     *         under which this service is available. If empty, the service does
     *         not provide D-Bus services. If the pointer is 0 the argument
     *         will be ignored
     * @param pid On success, the process id of the new service will be written
     *        here. If the pointer is 0, the argument will be ignored.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @param noWait if set, the function does not wait till the service is running.
     * @return an error code indicating success (== 0) or failure (> 0).
     * @deprecated since 5.0 use QDBusConnectionInterface::startService("org.kde.serviceName"),
     *   to start a unique application in order to make D-Bus calls to it (after ensuring that
     *   it installs a D-Bus org.kde.serviceName.service file). Otherwise just use QProcess,
     *   KIO::CommandLauncherJob or KIO::ApplicationLauncherJob.
     */
    KSERVICE_DEPRECATED_VERSION_BELATED(5, 82, 5, 0, "See API documentation")
    static int startServiceByDesktopName(const QString &_name,
                                         const QString &URL,
                                         QString *error = nullptr,
                                         QString *serviceName = nullptr,
                                         int *pid = nullptr,
                                         const QByteArray &startup_id = QByteArray(),
                                         bool noWait = false);
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 82)
    /**
     * Starts a service based on the desktop name of the service.
     * E.g. "konqueror"
     *
     * @param _name the desktop name of the service
     * @param URLs if not empty these URLs will be passed to the service
     * @param error On failure, @p error contains a description of the error
     *         that occurred. If the pointer is 0, the argument will be
     *         ignored
     * @param serviceName On success, @p serviceName contains the D-Bus service name
     *         under which this service is available. If empty, the service does
     *         not provide D-Bus services. If the pointer is 0 the argument
     *         will be ignored
     * @param pid On success, the process id of the new service will be written
     *        here. If the pointer is 0, the argument will be ignored.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @param noWait if set, the function does not wait till the service is running.
     * @return an error code indicating success (== 0) or failure (> 0).
     * @deprecated since 5.0 use QDBusConnectionInterface::startService("org.kde.serviceName"),
     *   to start a unique application in order to make D-Bus calls to it (after ensuring that
     *   it installs a D-Bus org.kde.serviceName.service file). Otherwise just use QProcess,
     *   KIO::CommandLauncherJob or KIO::ApplicationLauncherJob.
     */
    KSERVICE_DEPRECATED_VERSION_BELATED(5, 82, 5, 0, "See API documentation")
    static int startServiceByDesktopName(const QString &_name,
                                         const QStringList &URLs = QStringList(),
                                         QString *error = nullptr,
                                         QString *serviceName = nullptr,
                                         int *pid = nullptr,
                                         const QByteArray &startup_id = QByteArray(),
                                         bool noWait = false);
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 83)
    /**
     * Starts a program via kdeinit.
     *
     * program name and arguments are converted to according to the
     * local encoding and passed as is to kdeinit.
     *
     * @param name Name of the program to start
     * @param args Arguments to pass to the program
     * @param error On failure, @p error contains a description of the error
     *         that occurred. If the pointer is 0, the argument will be
     *         ignored
     * @param pid On success, the process id of the new service will be written
     *        here. If the pointer is 0, the argument will be ignored.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @return an error code indicating success (== 0) or failure (> 0).
     * @deprecated since 5.83, use CommandLauncherJob instead
     */
    KSERVICE_DEPRECATED_VERSION(5, 83, "use CommandLauncherJob instead")
    static int kdeinitExec(const QString &name,
                           const QStringList &args = QStringList(),
                           QString *error = nullptr,
                           int *pid = nullptr,
                           const QByteArray &startup_id = QByteArray());
#endif

    /**
     * Starts a program via kdeinit and wait for it to finish.
     *
     * Like kdeinitExec(), but it waits till the program is finished.
     * As such it behaves similar to the system(...) function.
     *
     * @param name Name of the program to start
     * @param args Arguments to pass to the program
     * @param error On failure, @p error contains a description of the error
     *         that occurred. If the pointer is 0, the argument will be
     *         ignored
     * @param pid On success, the process id of the new service will be written
     *        here. If the pointer is 0, the argument will be ignored.
     * @param startup_id for app startup notification, "0" for none,
     *           "" ( empty string ) is the default
     * @return an error code indicating success (== 0) or failure (> 0).
     * @deprecated since 5.88, kdeinit is deprecated, launch the executable manually
     */
    KSERVICE_DEPRECATED_VERSION(5, 89, "kdeinit is deprecated, launch the executable manually")
    static int kdeinitExecWait(const QString &name,
                               const QStringList &args = QStringList(),
                               QString *error = nullptr,
                               int *pid = nullptr,
                               const QByteArray &startup_id = QByteArray());

    /**
     * Ensures that kdeinit5 and klauncher are running.
     * @deprecated Since 5.88, kdeinit is deprecated
     */
    KSERVICE_DEPRECATED_VERSION(5, 89, "kdeinit is deprecated")
    static void ensureKdeinitRunning();

Q_SIGNALS:
    /**
     * Hook for KApplication in kdeui
     * @internal
     */
    void kapplication_hook(QStringList &env, QByteArray &startup_id);

private:
    int startServiceInternal(const char *_function,
                             const QString &_name,
                             const QStringList &URLs,
                             QString *error,
                             QString *serviceName,
                             int *pid,
                             const QByteArray &startup_id,
                             bool noWait,
                             const QString &workdir = QString(),
                             const QStringList &envs = QStringList());
    static bool isMainThreadActive(QString *error = nullptr);

    KToolInvocationPrivate *const d;
    friend class KToolInvocationSingleton;
};

#endif
#endif
