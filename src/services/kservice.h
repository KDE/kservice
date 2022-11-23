/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 1999-2006 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSERVICE_H
#define KSERVICE_H

#include "kserviceaction.h"
#include <QCoreApplication>
#include <QStringList>
#include <QVariant>
#include <ksycocaentry.h>

// Exclude includes that are not needed when deprecations are disabled
#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 94)
#include <KPluginFactory>
#include <KPluginLoader>
#endif

class KServiceType;
class QDataStream;
class KDesktopFile;
class QWidget;

class KServicePrivate;

/**
 * @class KService kservice.h <KService>
 *
 * Represent a service, like an application or plugin
 * bound to one or several MIME types (or servicetypes) as written
 * in its desktop entry file.
 *
 * The starting point you need is often the static methods, like createInstance().
 * The types of service a plugin provides is taken from the accompanying desktop file
 * where the 'X-KDE-ServiceTypes=' field is used.
 *
 * For a tutorial on how to build a plugin-loading mechanism and how to write plugins
 * in general, see http://techbase.kde.org/Development/Tutorials#Services:_Applications_and_Plugins
 *
 * @see KServiceType
 * @see KServiceGroup
 * @author Torben Weis
 */
class KSERVICE_EXPORT KService : public KSycocaEntry
{
public:
    /**
     * A shared data pointer for KService.
     */
    typedef QExplicitlySharedDataPointer<KService> Ptr;
    /**
     * A list of shared data pointers for KService.
     */
    typedef QList<Ptr> List;

    /**
     * Construct a temporary service with a given name, exec-line and icon.
     * @param name the name of the service
     * @param exec the executable
     * @param icon the name of the icon
     */
    KService(const QString &name, const QString &exec, const QString &icon);

    /**
     * Construct a service and take all information from a config file.
     *
     * @param fullpath Full path to the config file.
     */
    explicit KService(const QString &fullpath);

    /**
     * Construct a service and take all information from a desktop file.
     * @param config the desktop file to read
     * @param optional relative path to store for findByName
     */
    explicit KService(const KDesktopFile *config, const QString &entryPath = QString());

    KService(const KService &other);

    ~KService() override;

    /**
     * Services are either applications (executables) or dlopened libraries (plugins).
     * @return true if this service is an application, i.e. it has Type=Application in its
     * .desktop file and exec() will not be empty.
     */
    bool isApplication() const;

    /**
     * Returns the executable.
     * @return the command that the service executes,
     *         or QString() if not set
     */
    QString exec() const;
    /**
     * Returns the name of the service's library.
     * @return the name of the library that contains the service's
     *         implementation, or QString() if not set
     */
    QString library() const;

    /**
     * Returns the name of the icon.
     * @return the icon associated with the service,
     *         or QString() if not set
     */
    QString icon() const;
    /**
     * Checks whether the service should be run in a terminal.
     * @return true if the service is to be run in a terminal.
     */
    bool terminal() const;

    /**
     * Returns any options associated with the terminal the service
     * runs in, if it requires a terminal.
     *
     * The service must be a tty-oriented program.
     * @return the terminal options,
     *         or QString() if not set
     */
    QString terminalOptions() const;

    /**
     * Returns @c true if the service inidicates that it's preferred to run
     * the application on a discrete graphics card, otherwise return @c false.
     *
     * In releases older than 5.86 this methoed checked for the @c X-KDE-RunOnDiscreteGpu
     * key in the .desktop file represented by this service; starting from 5.86 this method
     * now also checks for @c PrefersNonDefaultGPU key (added to the Freedesktop.org desktop
     * entry spec in version 1.4 of the spec).
     *
     * @since 5.30
     */
    bool runOnDiscreteGpu() const;

    /**
     * Checks whether the service runs with a different user id.
     * @return true if the service has to be run under a different uid.
     * @see username()
     */
    bool substituteUid() const;
    /**
     * Returns the user name, if the service runs with a
     * different user id.
     * @return the username under which the service has to be run,
     *         or QString() if not set
     * @see substituteUid()
     */
    QString username() const;

    /**
     * Returns the filename of the service desktop entry without any
     * extension. E.g. "kppp"
     * @return the name of the desktop entry without path or extension,
     *         or QString() if not set
     */
    QString desktopEntryName() const;

    /**
     * Returns the menu ID of the service desktop entry.
     * The menu ID is used to add or remove the entry to a menu.
     * @return the menu ID
     */
    QString menuId() const;

    /**
     * Returns a normalized ID suitable for storing in configuration files.
     * It will be based on the menu-id when available and otherwise falls
     * back to entryPath()
     * @return the storage ID
     */
    QString storageId() const;

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 102)
    /**
     * Describes the D-Bus Startup type of the service.
     * @li None - This service has no D-Bus support
     * @li Unique - This service provides a unique D-Bus service.
     *              The service name is equal to the desktopEntryName.
     * @li Multi - This service provides a D-Bus service which can be run
     *             with multiple instances in parallel. The service name of
     *             an instance is equal to the desktopEntryName + "-" +
     *             the PID of the process.
     */
    enum DBusStartupType { DBusNone = 0, DBusUnique, DBusMulti };
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 102)
    /**
     * Returns the DBUSStartupType supported by this service.
     * @return the DBUSStartupType supported by this service
     *
     * @deprecated since 5.102, no known users.
     */
    KSERVICE_DEPRECATED_VERSION(5, 102, "No known users")
    DBusStartupType dbusStartupType() const;
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 63)
    /**
     * @return the working directory to run the program in,
     *         or QString() if not set
     * @deprecated since 5.63, use workingDirectory() instead
     */
    KSERVICE_DEPRECATED_VERSION(5, 63, "Use KService::workingDirectory()")
    QString path() const;
#endif

    /**
     * @return the working directory to run the program in,
     *         or QString() if not set
     * @since 5.63
     */
    QString workingDirectory() const;

    /**
     * Returns the descriptive comment for the service, if there is one.
     * @return the descriptive comment for the service, or QString()
     *         if not set
     */
    QString comment() const;

    /**
     * Returns the generic name for the service, if there is one
     * (e.g. "Mail Client").
     * @return the generic name,
     *         or QString() if not set
     */
    QString genericName() const;

    /**
     * Returns the untranslated (US English) generic name
     * for the service, if there is one
     * (e.g. "Mail Client").
     * @return the generic name,
     *         or QString() if not set
     */
    QString untranslatedGenericName() const;

    /**
     * Returns a list of descriptive keywords the service, if there are any.
     * @return the list of keywords
     */
    QStringList keywords() const;

    /**
     * Returns a list of VFolder categories.
     * @return the list of VFolder categories
     */
    QStringList categories() const;

    /**
     * Returns the list of MIME types that this service supports.
     * Note that this doesn't include inherited MIME types,
     * only the MIME types listed in the .desktop file.
     * @since 4.8.3
     */
    QStringList mimeTypes() const;

    /**
     * Returns the service types that this service supports.
     * @return the list of service types that are supported
     * Note that this doesn't include inherited servicetypes or MIME types,
     * only the service types listed in the .desktop file.
     */
    QStringList serviceTypes() const;

    /**
     * Checks whether the service supports this service type
     * @param serviceTypePtr The name of the service type you are
     *        interested in determining whether this service supports.
     *
     * @return true if the service type you specified is supported, otherwise false.
     */
    bool hasServiceType(const QString &serviceTypePtr) const;

    /**
     * Checks whether the service supports this MIME type
     * @param mimeType The name of the MIME type you are
     *        interested in determining whether this service supports.
     * @since 4.6
     */
    bool hasMimeType(const QString &mimeType) const;

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 67)
    /**
     * Set to true if it is allowed to use this service as the default (main)
     * action for the files it supports (e.g. Left Click in a file manager, or KRun in general).
     *
     * If not, then this service is only available in RMB popups, so it must
     * be selected explicitly by the user in order to be used.
     * Note that servicemenus supersede this functionality though, at least in konqueror.
     *
     * @return true if the service may be used as the default (main) handler
     * @deprecated since 5.67 due to no known use case
     */
    KSERVICE_DEPRECATED_VERSION(5, 67, "No known use case")
    bool allowAsDefault() const;
#endif

    /**
     * Returns the actions defined in this desktop file
     */
    QList<KServiceAction> actions() const;

    /**
     * Checks whether this service can handle several files as
     * startup arguments.
     * @return true if multiple files may be passed to this service at
     * startup. False if only one file at a time may be passed.
     */
    bool allowMultipleFiles() const;

    /**
     * What preference to associate with this service initially (before
     * the user has had any chance to define a profile for it).
     * The bigger the value, the most preferred the service is.
     * @return the service preference level of the service
     */
    int initialPreference() const;

    /**
     * Whether the entry should be suppressed in the K menu.
     * @return true to suppress this service
     *
     * Such services still appear in trader queries, i.e. in
     * "Open With" popup menus for instance.
     */
    bool noDisplay() const;

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 0)
    /**
     * Whether the service should be shown in KDE at all
     * (including in context menus).
     * @return true if the service should be shown.
     *
     * KMimeTypeTrader honors this and removes such services
     * from its results.
     *
     * @since 4.5
     * @deprecated since 5.0, use showInCurrentDesktop()
     */
    KSERVICE_DEPRECATED_VERSION(5, 0, "Use KService::showInCurrentDesktop()")
    bool showInKDE() const
    {
        return showInCurrentDesktop();
    }
#endif

    /**
     * Whether the service should be shown in the current desktop
     * (including in context menus).
     * @return true if the service should be shown.
     *
     * KApplicationTrader honors this and removes such services
     * from its results.
     *
     * @since 5.0
     */
    bool showInCurrentDesktop() const;

    /**
     * Whether the service should be shown on the current
     * platform (e.g. on xcb or on wayland).
     * @return true if the service should be shown
     *
     * @since 5.0
     */
    bool showOnCurrentPlatform() const;

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 87)
    /**
     * Name of the application this service belongs to.
     * (Useful for e.g. plugins)
     * @return the parent application, or QString() if not set
     * @deprecated Since 5.87, the concept of parent apps is deprecated. Plugins should use KPluginMetaData instead of
     * KService and a dedicated namespace if the plugins are only for one app relevant.
     */
    KSERVICE_DEPRECATED_VERSION(5, 87, "See API docs")
    QString parentApp() const;
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 87)
    /**
     * The keyword to be used when constructing the plugin using KPluginFactory. The keyword is
     * defined with X-KDE-PluginKeyword in the .desktop file and with registerPlugin<T>(keyword)
     * in the K_PLUGIN_FACTORY macro when implementing the plugin.
     * @deprecated Since 5.87, the metadata should be embedded in the actual plugin. Consequently
     * this property is obsolete. In case there is only one plugin in the library the usage is not needed.
     * In case there are different base classes registered the keyword is not needed too.
     * If there are multiple classes of a common base class registered they should be split up
     * in separate libs.
     */
    KSERVICE_DEPRECATED_VERSION(5, 87, "See API docs")
    QString pluginKeyword() const;
#endif

    /**
     * The path to the documentation for this service.
     * @since 4.2
     * @return the documentation path, or QString() if not set
     */
    QString docPath() const;

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 102)
    /**
     * Returns the requested property.
     *
     * @param _name the name of the property
     * @param t the assumed type of the property
     * @return the property, or invalid if not found
     * @see KServiceType
     *
     * @deprecated since 5.102, use property(QString, QMetaType::Type) instead.
     */
    KSERVICE_DEPRECATED_VERSION(5, 102, "Use property(QString, QMetaType::Type) instead")
    QVariant property(const QString &_name, QVariant::Type t) const;
#endif

    /**
     * Returns the requested property.
     *
     * @param _name the name of the property
     * @param t the assumed type of the property
     * @return the property, or invalid if not found
     * @see KServiceType
     *
     * @since 5.102
     */
    QVariant property(const QString &_name, QMetaType::Type t) const;

    using KSycocaEntry::property;

    /**
     * Returns a path that can be used for saving changes to this
     * service
     * @return path that can be used for saving changes to this service
     */
    QString locateLocal() const;

    /**
     * @internal
     * Set the menu id
     */
    void setMenuId(const QString &menuId);
    /**
     * @internal
     * Sets whether to use a terminal or not
     */
    void setTerminal(bool b);
    /**
     * @internal
     * Sets the terminal options to use
     */
    void setTerminalOptions(const QString &options);

    /**
     * Overrides the "Exec=" line of the service.
     *
     * If @ref exec is not empty, its value will override the one
     * the one set when this service was created.
     *
     * Please note that @ref entryPath is also cleared so the service
     * will no longer be associated with a specific config file.
     *
     * @internal
     * @since 4.11
     */
    void setExec(const QString &exec);

    /**
     * Overrides the "Path=" line of the service.
     *
     * If @ref workingDir is not empty, its value will override the one
     * the one set when this service was created.
     *
     * Please note that @ref entryPath is also cleared so the service
     * will no longer be associated with a specific config file.
     *
     * @internal
     * @param workingDir
     * @since 5.79
     */
    void setWorkingDirectory(const QString &workingDir);

    /**
     * Find a service based on its path as returned by entryPath().
     * It's usually better to use serviceByStorageId() instead.
     *
     * @param _path the path of the configuration file
     * @return a pointer to the requested service or @c nullptr if the service is
     *         unknown.
     * @em Very @em important: Don't store the result in a KService* !
     */
    static Ptr serviceByDesktopPath(const QString &_path);

    /**
     * Find a service by the name of its desktop file, not depending on
     * its actual location (as long as it's under the applications or service
     * directories). For instance "konqbrowser" or "kcookiejar". Note that
     * the ".desktop" extension is implicit.
     *
     * This is the recommended method (safe even if the user moves stuff)
     * but note that it assumes that no two entries have the same filename.
     *
     * @param _name the name of the configuration file
     * @return a pointer to the requested service or @c nullptr if the service is
     *         unknown.
     * @em Very @em important: Don't store the result in a KService* !
     */
    static Ptr serviceByDesktopName(const QString &_name);

    /**
     * Find a service by its menu-id
     *
     * @param _menuId the menu id of the service
     * @return a pointer to the requested service or @c nullptr if the service is
     *         unknown.
     * @em Very @em important: Don't store the result in a KService* !
     */
    static Ptr serviceByMenuId(const QString &_menuId);

    /**
     * Find a service by its storage-id or desktop-file path. This
     * function will try very hard to find a matching service.
     *
     * @param _storageId the storage id or desktop-file path of the service
     * @return a pointer to the requested service or @c nullptr if the service is
     *         unknown.
     * @em Very @em important: Don't store the result in a KService* !
     */
    static Ptr serviceByStorageId(const QString &_storageId);

    /**
     * Returns the whole list of services.
     *
     *  Useful for being able to
     * to display them in a list box, for example.
     * More memory consuming than the ones above, don't use unless
     * really necessary.
     * @return the list of all services
     */
    static List allServices();

    /**
     * Returns a path that can be used to create a new KService based
     * on @p suggestedName.
     * @param showInMenu true, if the service should be shown in the KDE menu
     *        false, if the service should be hidden from the menu
     *        This argument isn't used anymore, use NoDisplay=true to hide the service.
     * @param suggestedName name to base the file on, if a service with such a
     *        name already exists, a suffix will be added to make it unique
     *        (e.g. foo.desktop, foo-1.desktop, foo-2.desktop).
     * @param menuId If provided, menuId will be set to the menu id to use for
     *        the KService
     * @param reservedMenuIds If provided, the path and menu id will be chosen
     *        in such a way that the new menu id does not conflict with any
     *        of the reservedMenuIds
     * @return The path to use for the new KService.
     */
    static QString newServicePath(bool showInMenu, const QString &suggestedName, QString *menuId = nullptr, const QStringList *reservedMenuIds = nullptr);

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 86)
#if KCOREADDONS_ENABLE_DEPRECATED_SINCE(5, 86)
    /**
     * This template allows to load the library for the specified service and ask the
     * factory to create an instance of the given template type.
     *
     * @param parent The parent object (see QObject constructor)
     * @param args A list of arguments, passed to the factory and possibly
     *             to the component (see KPluginFactory)
     * @param error A pointer to QString which should receive the error description or @c nullptr
     *
     * @return A pointer to the newly created object or a null pointer if the
     *         factory was unable to create an object of the given type.
     *
     * @see KPluginFactory::instantiatePlugin
     * @deprecated Since 5.86, Use KPluginMetaData/KPluginFactory or QPluginloader instead
     */
    template<class T>
    KSERVICE_DEPRECATED_VERSION(5, 86, "Use KPluginMetaData/KPluginFactory or QPluginloader instead")
    T *createInstance(QObject *parent = nullptr, const QVariantList &args = QVariantList(), QString *error = nullptr) const
    {
        return createInstance<T>(nullptr, parent, args, error);
    }
#endif
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 86)
#if KCOREADDONS_ENABLE_DEPRECATED_SINCE(5, 86)
    /**
     * This template allows to load the library for the specified service and ask the
     * factory to create an instance of the given template type.
     *
     * @param parentWidget A parent widget for the service. This is used e. g. for parts
     * @param parent The parent object (see QObject constructor)
     * @param args A list of arguments, passed to the factory and possibly
     *             to the component (see KPluginFactory)
     * @param error A pointer to QString which should receive the error description or @c nullptr
     *
     * @return A pointer to the newly created object or a null pointer if the
     *         factory was unable to create an object of the given type.
     *
     * @see KPluginFactory::instantiatePlugin
     * @deprecated Since 5.86, Use KPluginMetaData/KPluginFactory or QPluginloader instead
     */
    template<class T>
    KSERVICE_DEPRECATED_VERSION(5, 86, "Use KPluginMetaData/KPluginFactory or QPluginloader instead")
    T *createInstance(QWidget *parentWidget, QObject *parent, const QVariantList &args = QVariantList(), QString *error = nullptr) const
    {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_DEPRECATED
        KPluginLoader pluginLoader(*this);
        KPluginFactory *factory = pluginLoader.factory();
        if (factory) {
            QVariantList argsWithMetaData = args;
            argsWithMetaData << pluginLoader.metaData().toVariantMap();
            T *o = factory->template create<T>(parentWidget, parent, pluginKeyword(), argsWithMetaData);
            if (!o && error)
                *error = QCoreApplication::translate("", "The service '%1' does not provide an interface '%2' with keyword '%3'")
                             .arg(name(), QString::fromLatin1(T::staticMetaObject.className()), pluginKeyword());
            return o;
        } else if (error) {
            *error = pluginLoader.errorString();
            pluginLoader.unload();
        }
        QT_WARNING_POP
        return nullptr;
    }
#endif
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 86)
#if KCOREADDONS_ENABLE_DEPRECATED_SINCE(5, 86)
    /**
     * Convert this KService to a KPluginName.
     *
     * This allows expressions like
     * @code
     * KPluginLoader(service);
     * @endcode
     * which will use library() as the name of the plugin.  If the service
     * is not valid or does not have a library, KPluginLoader::errorString()
     * will be set appropriately.
     */
    operator KPluginName() const;
#endif
#endif

    /**
     * @brief A desktop file name that this service is an alias for.
     *
     * This is used when a NoDisplay service is used to enforce specific handling
     * for an application. In that case the NoDisplay service is an AliasFor another
     * service and be considered roughly equal to the AliasFor service (which should
     * not be NoDisplay=true)
     * For example okular supplies a desktop file for each supported format (e.g. PDF), all
     * of which NoDisplay and merely there to selectively support specific file formats.
     * A UI may choose to display the aliased entry org.kde.okular instead of the NoDisplay entries.
     *
     * @since 5.96
     *
     * @return QString desktopName of the aliased service (excluding .desktop suffix)
     */
    QString aliasFor() const;

private:
    friend class KBuildServiceFactory;

    /// @internal for KBuildSycoca only
    struct ServiceTypeAndPreference {
        ServiceTypeAndPreference()
            : preference(-1)
            , serviceType()
        {
        }
        ServiceTypeAndPreference(int pref, const QString &servType)
            : preference(pref)
            , serviceType(servType)
        {
        }
        int preference;
        QString serviceType; // or MIME type
    };
    /// @internal for KBuildSycoca only
    QVector<ServiceTypeAndPreference> &_k_accessServiceTypes();

    friend QDataStream &operator>>(QDataStream &, ServiceTypeAndPreference &);
    friend QDataStream &operator<<(QDataStream &, const ServiceTypeAndPreference &);

    Q_DECLARE_PRIVATE(KService)

    friend class KServiceFactory;

    /**
     * @internal
     * Construct a service from a stream.
     * The stream must already be positioned at the correct offset.
     */
    KService(QDataStream &str, int offset);
};
#endif
