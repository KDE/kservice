/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSERVICEACTION_H
#define KSERVICEACTION_H

#include <QSharedDataPointer>
#include <QVariant>
#include <kservice_export.h>
class QVariant;
class KServiceActionPrivate;
class KService;

// we can't include kservice.h, it includes this header...
typedef QExplicitlySharedDataPointer<KService> KServicePtr;

/**
 * @class KServiceAction kserviceaction.h <KServiceAction>
 *
 * Represents an action in a .desktop file
 * Actions are defined with the config key Actions in the [Desktop Entry]
 * group, followed by one group per action, as per the desktop entry standard.
 * @see KService::actions
 */
class KSERVICE_EXPORT KServiceAction
{
public:
#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 69)
    /**
     * Creates a KServiceAction.
     * Normally you don't have to do this, KService creates the actions
     * when parsing the .desktop file.
     * @deprecated Since 5.69, use the 6-args constructor
     */
    KSERVICE_DEPRECATED_VERSION_BELATED(5, 71, 5, 69, "Use the 6-args constructor")
    KServiceAction(const QString &name, const QString &text, const QString &icon, const QString &exec, bool noDisplay = false);
#endif
    /**
     * Creates a KServiceAction.
     * Normally you don't have to do this, KService creates the actions
     * when parsing the .desktop file.
     * @since 5.69
     */
    KServiceAction(const QString &name, const QString &text, const QString &icon, const QString &exec, bool noDisplay, const KServicePtr &service);
    /**
     * @internal
     * Needed for operator>>
     */
    KServiceAction();
    /**
     * Destroys a KServiceAction.
     */
    ~KServiceAction();

    /**
     * Copy constructor
     */
    KServiceAction(const KServiceAction &other);
    /**
     * Assignment operator
     */
    KServiceAction &operator=(const KServiceAction &other);

    /**
     * Sets the action's internal data to the given @p userData.
     */
    void setData(const QVariant &userData);
    /**
     * @return the action's internal data.
     */
    QVariant data() const;

    /**
     * @return the action's internal name
     * For instance Actions=Setup;... and the group [Desktop Action Setup]
     * define an action with the name "Setup".
     */
    QString name() const;

    /**
     * @return the action's text, as defined by the Name key in the desktop action group
     */
    QString text() const;

    /**
     * @return the action's icon, as defined by the Icon key in the desktop action group
     */
    QString icon() const;

    /**
     * @return the action's exec command, as defined by the Exec key in the desktop action group
     */
    QString exec() const;

    /**
     * Returns whether the action should be suppressed in menus.
     * This is useful for having actions with a known name that the code
     * looks for explicitly, like Setup and Root for kscreensaver actions,
     * and which should not appear in popup menus.
     * @return true to suppress this service
     */
    bool noDisplay() const;

    /**
     * Returns whether the action is a separator.
     * This is true when the Actions key contains "_SEPARATOR_".
     */
    bool isSeparator() const;

    /**
     * Returns the service that this action comes from
     * @since 5.69
     */
    KServicePtr service() const;

    /**
     * Returns the requested property.
     *
     * @param name the name of the property
     * @param type the assumed type of the property
     * @return the property, or an invalid QVariant if not found
     * @since 5.102
     */
    QVariant property(const QString &name, QMetaType::Type type) const;

private:
    QSharedDataPointer<KServiceActionPrivate> d;
    friend KSERVICE_EXPORT QDataStream &operator>>(QDataStream &str, KServiceAction &act);
    friend KSERVICE_EXPORT QDataStream &operator<<(QDataStream &str, const KServiceAction &act);
    friend class KService;
    void setService(const KServicePtr &service);
};

KSERVICE_EXPORT QDataStream &operator>>(QDataStream &str, KServiceAction &act);
KSERVICE_EXPORT QDataStream &operator<<(QDataStream &str, const KServiceAction &act);

#endif /* KSERVICEACTION_H */
