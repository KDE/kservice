/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KDELIBS_KAUTOSTART_H
#define KDELIBS_KAUTOSTART_H

#include <kservice_export.h>

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 87)

#include <QObject>
#include <QStringList>
#include <memory>

class KAutostartPrivate;

/**
 * @class KAutostart kautostart.h <KAutostart>
 *
 * KAutostart provides a programmatic means to control the state of
 * autostart services on a per-user basis. This is useful for applications
 * that wish to offer a configurable means to allow the application to be
 * autostarted.
 *
 * By using this class you future-proof your applications against potential
 * future or platform-specific changes to the autostart mechanism(s).
 *
 * Typical usage might look like:
 *
 * @code
 * KAutostart autostart; // without an entryName arg, gets name from KAboutData
 * autostart.setAutostarts(true); // will now start up when the user logs in
 *
 * // set the value in our configuration settings to reflect whether or not
 * // we will actually start up on log in
 * config.setAutoStart(autostart.autoStarts());
 * @endcode
 * @deprecated Since 5.87, the autostart is done inside of Plasma, apps should use the X-KDE-autostart-condition
 * to toggle their autostart values.
 * If the app should autostart in other desktop environments it's desktop file has to be compied to the autostart location manually.
 */
class KSERVICE_EXPORT KAutostart : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a new KAutostart object that represents the autostart
     * service "entryName". If the service already exists in the system
     * then the values associated with that service, such as the executable
     * command, will be loaded as well.
     *
     * Note that unless this service is explicitly set to autostart,
     * simply creating a KAutostart object will not result in the
     * service being autostarted on next log in.
     *
     * If no such service is already registered and the command to be
     * executed on startup is not the same as entryName, then you will want
     * to set the associated command with setExec(const QString&)
     * @see setExec
     * @param entryName the name used to identify the service. If none is
     *        provided then it uses the name registered with KAboutData.
     * @param parent QObject
     *
     * @since 5.61 we are allowed to specify an absolute path to the service
     * description and it will still work.
     * @deprecated Since 5.87, see class docs
     */
    KSERVICE_DEPRECATED_VERSION(5, 87, "See class docs")
    explicit KAutostart(const QString &entryName = QString(), QObject *parent = nullptr);
    ~KAutostart() override;

    /**
     * Flags for each of the conditions that may affect whether or not
     * a service actually autostarted on login
     * @see Conditions
     */
    enum Condition {
        /**
         * no conditions set
         */
        NoConditions = 0x0,
        /**
         * an executable that is checked for existence by name
         */
        CheckCommand = 0x1,
        /**
         * autostart condition will be checked too (KDE-specific)
         * @since 4.3
         */
        CheckCondition = 0x2,
        /**
         * all necessary conditions will be checked
         * @since 4.3
         */
        CheckAll = 0xff,
    };
    /**
     * Stores a combination of #Condition values.
     */
    Q_DECLARE_FLAGS(Conditions, Condition)

    /**
     * Enumerates the various autostart phases that occur during start-up.
     */
    enum StartPhase {
        /**
         * the essential desktop services such as panels and window managers
         */
        BaseDesktop = 0,
        /**
         * services that should be available before most interactive
         * applications start but that aren't part of the base desktop.
         * This would include things such as clipboard managers and
         * mouse gesture tools.
         */
        DesktopServices = 1,
        /**
         * everything else that doesn't belong in the above two categories,
         * including most system tray applications, system monitors and
         * interactive applications
         */
        Applications = 2,
    };

    /**
     * Sets the given exec to start automatically at login
     * @param autostart will register with the autostart facility when true
     *        and deregister when false
     * @see autostarts()
     */
    void setAutostarts(bool autostart);

    /**
     * Returns whether or not the service represented by entryName in the
     * autostart system is set to autostart at login or not
     * @param environment if provided the check will be performed as if
     *        being loaded in that environment
     * @param check autostart conditions to check for (see commandToCheck())
     * @see setAutostarts()
     */
    bool autostarts(const QString &environment = QString(), Conditions check = NoConditions) const;

    /**
     * Returns the associated command for this autostart service
     * @see setCommand()
     */
    QString command() const;
    /**
     * Set the associated command for this autostart service
     * @see command()
     */
    void setCommand(const QString &command);

    /**
     * Returns the user-visible name this autostart service is registered as
     * @see setVisibleName(), setEntryName()
     */
    QString visibleName() const;
    /**
     * Sets the user-visible name for this autostart service.
     * @see visibleName()
     */
    void setVisibleName(const QString &entryName);

    /**
     * Checks whether or not a service by the given name @p entryName is registered
     * with the autostart system. Does not check whether or not it is
     * set to actually autostart or not.
     * @param entryName the name of the service to check for
     * @deprecated Since 5.87, see class docs
     */
    KSERVICE_DEPRECATED_VERSION(5, 87, "See class docs")
    static bool isServiceRegistered(const QString &entryName);

    /**
     * Returns the executable to check for when attempting to autostart
     * this service. If the executable is not found in the user's
     * environment, it will not autostart.
     * @see setCommandToCheck()
     */
    QString commandToCheck() const;
    /**
     * Sets the executable to check for the existence of when
     * autostarting this service
     * @see commandToCheck()
     */
    void setCommandToCheck(const QString &exec);

    /**
     * Returns the autostart phase this service is started in.
     *
     * Note that this is KDE specific and may not work in other
     * environments.
     *
     * @see StartPhase, setStartPhase()
     */
    StartPhase startPhase() const;
    /**
     * Sets the service (by name) this service should be started after.
     *
     * Note that this is KDE specific and may not work in other
     * environments.
     *
     * @see StartPhase, startPhase()
     */
    void setStartPhase(StartPhase phase);

    /**
     * Returns the list of environments (e.g. "KDE") this service is allowed
     * to start in. Use checkAllowedEnvironment() or autostarts() for actual
     * checks.
     *
     * This does not take other autostart conditions
     * into account. If any environment is added to the allowed environments
     * list, then only those environments will be allowed to
     * autoload the service. It is not allowed to specify both allowed and excluded
     * environments at the same time.
     * @see setAllowedEnvironments()
     */
    QStringList allowedEnvironments() const;
    /**
     * Sets the environments this service is allowed to start in
     * @see allowedEnvironments(), addToAllowedEnvironments()
     */
    void setAllowedEnvironments(const QStringList &environments);
    /**
     * Adds an environment to the list of environments this service may
     * start in.
     * @see setAllowedEnvironments(), removeFromAllowedEnvironments()
     */
    void addToAllowedEnvironments(const QString &environment);
    /**
     * Removes an environment to the list of environments this service may
     * start in.
     * @see addToAllowedEnvironments()
     */
    void removeFromAllowedEnvironments(const QString &environment);

    /**
     * Returns the list of environments this service is explicitly not
     * allowed to start in. Use checkAllowedEnvironment() or autostarts() for actual
     * checks.
     *
     * This does not take other autostart conditions
     * such as into account. It is not allowed to specify both allowed and excluded
     * environments at the same time.
     * @see setExcludedEnvironments()
     */
    QStringList excludedEnvironments() const;
    /**
     * Sets the environments this service is not allowed to start in
     * @see excludedEnvironments(), addToExcludedEnvironments()
     */
    void setExcludedEnvironments(const QStringList &environments);
    /**
     * Adds an environment to the list of environments this service may
     * not be autostarted in
     * @see removeFromExcludedEnvironments()
     */
    void addToExcludedEnvironments(const QString &environment);
    /**
     * Removes an environment to the list of environments this service may
     * not be autostarted in
     * @see addToExcludedEnvironments()
     */
    void removeFromExcludedEnvironments(const QString &environment);

    /**
     * Returns the name of another service that should be autostarted
     * before this one (if that service would be autostarted).
     * @internal
     * @since 4.3
     */
    QString startAfter() const;

    /**
     * Checks whether autostart is allowed in the given environment,
     * depending on allowedEnvironments() and excludedEnvironments().
     * @since 4.3
     */
    bool checkAllowedEnvironment(const QString &environment) const;

    /**
     * Checks that a given autostart configuration condition is met.
     * @param condition: config in the format "rcfile:group:entry:default"
     *
     * @since 5.69
     * @deprecated Since 5.87, see class docs
     */
    KSERVICE_DEPRECATED_VERSION(5, 87, "See class docs")
    static bool isStartConditionMet(const QString &condition);

private:
    bool checkStartCondition() const;

private:
    std::unique_ptr<KAutostartPrivate> const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KAutostart::Conditions)
#endif
#endif
