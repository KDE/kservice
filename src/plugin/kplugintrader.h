/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef __kplugintrader_h__
#define __kplugintrader_h__

#include "kplugininfo.h"

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 82)

class KPluginTraderPrivate;
/**
 * \class KPluginTrader kplugintrader.h <KPluginTrader>
 *
 * A trader interface which provides a way to query specific subdirectories in the Qt
 * plugin paths for plugins. KPluginTrader provides an easy way to load a plugin
 * instance from a KPluginFactory, or just querying for existing plugins.
 *
 * KPluginTrader provides a way for an application to query directories in the
 * Qt plugin paths, accessed through QCoreApplication::libraryPaths().
 * Plugins may match a specific set of requirements. This allows to find
 * specific plugins at run-time without having to hard-code their names and/or
 * paths. KPluginTrader does not search recursively, you are rather encouraged
 * to install plugins into specific subdirectories to further speed searching.
 *
 * KPluginTrader exclusively searches within the plugin binaries' metadata
 * (via QPluginLoader::metaData()). It does not search these directories recursively.
 *
 * KPluginTrader does not use KServiceTypeTrader or KSyCoCa. As such, it will
 * only find binary plugins. If you are looking for a generic way to query for
 * services, use KServiceTypeTrader. For anything relating to MIME types (type
 * of files), use KMimeTypeTrader.
 *
 * \par Example
 *
 * If you want to find all plugins for your application,
 * you would define a KMyApp/Plugin servicetype, and then you can query
 * the trader for it:
 * \code
 * KPluginInfo::List offers =
 *     KPluginTrader::self()->query("KMyApp/Plugin", "kf5");
 * \endcode
 *
 * You can add a constraint in the "trader query language". For instance:
 * \code
 * KPluginTrader::self()->query("KMyApp/Plugin", "kf5",
 *                                   "[X-KMyApp-InterfaceVersion] > 15");
 * \endcode
 *
 * Please note that when including property names containing arithmetic operators like - or +, then you have
 * to put brackets around the property name, in order to correctly separate arithmetic operations from
 * the name. So for example a constraint expression like
 * \code
 * X-KMyApp-InterfaceVersion > 4 // wrong!
 * \endcode
 * needs to be written as
 * \code
 * [X-KMyApp-InterfaceVersion] > 4
 * \endcode
 * otherwise it could also be interpreted as
 * Subtract the numeric value of the property "KMyApp" and "InterfaceVersion" from the
 * property "X" and make sure it is greater than 4.\n
 * Instead of the other meaning, make sure that the numeric value of "X-KMyApp-InterfaceVersion" is
 * greater than 4.
 *
 * @see KMimeTypeTrader, KServiceTypeTrader, KPluginInfo
 * @see QCoreApplication::libraryPaths
 * @see QT_PLUGIN_PATH (env variable)
 * @see KPluginFactory
 * @see kservice_desktop_to_json (Cmake macro)
 * @see K_PLUGIN_FACTORY_WITH_JSON (macro defined in KPluginFactory)
 *
 * @since 5.0
 */
class KSERVICE_EXPORT KPluginTrader
{
public:
    /**
     * Standard destructor
     */
    ~KPluginTrader();

    /**
     * The main function in the KPluginTrader class.
     *
     * It will return a list of plugins that match your specifications. Required parameter is the
     * service type and subdirectory. This method will append the subDirectory to every path found
     * in QCoreApplication::libraryPaths(), append the subDirectory parameter, and search through
     * the plugin's metadata
     *
     * KPluginTrader exclusively searches within the plugin binaries' metadata
     * (via QPluginLoader::metaData()). It does not search these directories recursively.
     *
     * The constraint parameter is used to limit the possible choices returned based on the
     * constraints you give it.
     *
     * The @p constraint language is rather full.  The most common
     * keywords are AND, OR, NOT, IN, and EXIST, all used in an
     * almost spoken-word form.  An example is:
     * \code
     * (Type == 'Service') and (('KParts/ReadOnlyPart' in ServiceTypes) or (exist Exec))
     * \endcode
     *
     * If you want to load a list of plugins from a specific subdirectory, you can do the following:
     *
     * \code
     *
     * const KPluginInfo::List plugins = KPluginTrader::self()->query("plasma/engines");
     *
     * for (const KPluginInfo &info : plugins) {
     *      KPluginLoader loader(info.libraryPath());
     *      const QVariantList argsWithMetaData = QVariantList() << loader.metaData().toVariantMap();
     *      // In many cases, plugins are actually based on KPluginFactory, this is how that works:
     *      KPluginFactory* factory = loader.factory();
     *      if (factory) {
     *          Engine* component = factory->create<Engine>(parent, argsWithMetaData);
     *          if (component) {
     *              // Do whatever you want to do with the resulting object
     *          }
     *      }
     *      // Otherwise, just use the normal QPluginLoader methods
     *      Engine *myengine = qobject_cast<Engine*>(loader.instance());
     *      if (myengine) {
     *          // etc. ...
     *      }
     * }
     * \endcode
     *
     * If you have a specific query for just one plugin, use the createInstanceFromQuery method.
     *
     * The keys used in the query (Type, ServiceType, Exec) are all fields found in the .json files
     * which are compiled into the plugin binaries.
     *
     * @param subDirectory The subdirectory under the Qt plugin path
     * @param servicetype A service type like 'KMyApp/Plugin' or 'KFilePlugin'
     * @param constraint  A constraint to limit the choices returned, QString() to
     *                    get all services of the given @p servicetype
     *
     * @return A list of services that satisfy the query
     * @see http://techbase.kde.org/Development/Tutorials/Services/Traders#The_KTrader_Query_Language
     * @deprecated since 5.82, use KPluginMetaData::findPlugins.
     */
    KSERVICE_DEPRECATED_VERSION(5, 82, "Use KPluginMetaData::findPlugins")
    KPluginInfo::List query(const QString &subDirectory, const QString &serviceType = QString(), const QString &constraint = QString());

    /**
     * This is a static pointer to the KPluginTrader singleton.
     *
     * You will need to use this to access the KPluginTrader functionality since the
     * constructors are protected.
     *
     * @deprecated since 5.82, use KPluginMetaData and KPluginFactory.
     *
     * @return Static KPluginTrader instance
     */
    KSERVICE_DEPRECATED_VERSION(5, 82, "Use KPluginMetaData and KPluginFactory")
    static KPluginTrader *self();

    /**
     * Get a plugin from a trader query
     *
     * Example:
     * \code
     * KMyAppPlugin* plugin = KPluginTrader::createInstanceFromQuery<KMyAppPlugin>(subDirectory, serviceType, QString(), parentObject );
     * if ( plugin ) {
     *     ....
     * }
     * \endcode
     *
     * @param subDirectory The subdirectory under the Qt plugin paths to search in
     * @param serviceType The type of service for which to find a plugin
     * @param constraint An optional constraint to pass to the trader (see KTrader)
     * @param parent The parent object for the part itself
     * @param args A list of arguments passed to the service component
     * @param error The string passed here will contain an error description.
     * @return A pointer to the newly created object or a null pointer if the
     *         factory was unable to create an object of the given type.
     * @deprecated since 5.82, use KPluginLoader API.
     */
    template<class T>
    KSERVICE_DEPRECATED_VERSION(5, 82, "Use KPluginLoader API")
    static T *createInstanceFromQuery(const QString &subDirectory,
                                      const QString &serviceType = QString(),
                                      const QString &constraint = QString(),
                                      QObject *parent = nullptr,
                                      const QVariantList &args = QVariantList(),
                                      QString *error = nullptr)
    {
        return createInstanceFromQuery<T>(subDirectory, serviceType, constraint, parent, nullptr, args, error);
    }

    /**
     * Get a plugin from a trader query
     *
     * This method works like
     * createInstanceFromQuery(const QString&, const QString& ,const QString&, QObject*,
     * const QVariantList&, QString*),
     * but you can specify an additional parent widget.  This is important for a KPart, for example.
     *
     * @param subDirectory The subdirectory under the Qt plugin paths to search in
     * @param serviceType the type of service for which to find a plugin
     * @param constraint an optional constraint to pass to the trader (see KTrader)
     * @param parent the parent object for the part itself
     * @param parentWidget the parent widget for the plugin
     * @param args A list of arguments passed to the service component
     * @param error The string passed here will contain an error description.
     * @return A pointer to the newly created object or a null pointer if the
     *         factory was unable to create an object of the given type.
     * @deprecated since 5.82, use KPluginLoader API.
     */
    template<class T>
    KSERVICE_DEPRECATED_VERSION(5, 82, "Use KPluginLoader API")
    static T *createInstanceFromQuery(const QString &subDirectory,
                                      const QString &serviceType,
                                      const QString &constraint,
                                      QObject *parent,
                                      QWidget *parentWidget,
                                      const QVariantList &args = QVariantList(),
                                      QString *error = nullptr)
    {
        Q_UNUSED(parentWidget)
        Q_UNUSED(args)
        if (error) {
            error->clear();
        }
        const KPluginInfo::List offers = self()->query(subDirectory, serviceType, constraint);

        for (const KPluginInfo &info : offers) {
            KPluginLoader loader(info.libraryPath());
            const QVariantList argsWithMetaData = QVariantList() << loader.metaData().toVariantMap();
            KPluginFactory *factory = loader.factory();
            if (factory) {
                T *component = factory->create<T>(parent, argsWithMetaData);
                if (component) {
                    return component;
                }
            }
        }
        if (error && error->isEmpty()) {
            *error = QCoreApplication::translate("", "No service matching the requirements was found");
        }
        return nullptr;
    }

    KSERVICE_DEPRECATED_VERSION(5, 82, "No users.")
    static void applyConstraints(KPluginInfo::List &lst, const QString &constraint);

private:
    /**
     * @internal
     */
    KPluginTrader();

    // disallow copy ctor and assignment operator
    KPluginTrader(const KPluginTrader &other);
    KPluginTrader &operator=(const KPluginTrader &rhs);

    KPluginTraderPrivate *const d;
};
#endif

#endif
