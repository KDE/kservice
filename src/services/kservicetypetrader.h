/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef __kservicetypetrader_h__
#define __kservicetypetrader_h__

#include "kservice.h"

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 90)

class KServiceOffer;
typedef QList<KServiceOffer> KServiceOfferList;
class KServiceTypeTraderPrivate;
/**
 * @class KServiceTypeTrader kservicetypetrader.h <KServiceTypeTrader>
 *
 * KDE's trader interface (similar to the CORBA Trader), which provides a way
 * to query the KDE infrastructure for specific applications or components.
 *
 * Basically, KServiceTypeTrader provides a way for an application to query
 * all KDE services (that is, applications, components, plugins) that match
 * a specific set of requirements. This allows to find specific services
 * at run-time without having to hard-code their names and/or paths.
 *
 * For anything relating to MIME types (type of files), ignore KServiceTypeTrader
 * and use KMimeTypeTrader instead.
 *
 * \par Example
 *
 * If you want to find all plugins for your application,
 * you would define a KMyApp/Plugin servicetype, and then you can query
 * the trader for it:
 * \code
 * KService::List offers =
 *     KServiceTypeTrader::self()->query("KMyApp/Plugin");
 * \endcode
 *
 * You can add a constraint in the "trader query language". For instance:
 * \code
 * KServiceTypeTrader::self()->query("KMyApp/Plugin",
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
 * property "X" and make sure it is greater than 4.
 * Instead of the other meaning, make sure that the numeric value of "X-KMyApp-InterfaceVersion" is
 * greater than 4.
 *
 * @see KMimeTypeTrader, KService
 * @deprecated Since 5.90.
 * For querying plugins use @p KPluginMetaData::findPlugins.
 * For querying applications use @p KApplicationTrader.
 * For querying KParts use @p KParts::PartLoader.
 * For querying desktop files use @p KFileUtils::findAllUniqueFiles and @p KDesktopFile.
 */
class KSERVICE_EXPORT KServiceTypeTrader
{
public:
    /**
     * Standard destructor
     */
    ~KServiceTypeTrader();

    /**
     * The main function in the KServiceTypeTrader class.
     *
     * It will return a list of services that match your
     * specifications.  The only required parameter is the service
     * type.  This is something like 'text/plain' or 'text/html'.  The
     * constraint parameter is used to limit the possible choices
     * returned based on the constraints you give it.
     *
     * The @p constraint language is rather full.  The most common
     * keywords are AND, OR, NOT, IN, and EXIST, all used in an
     * almost spoken-word form.  An example is:
     * \code
     * (Type == 'Service') and (('KParts/ReadOnlyPart' in ServiceTypes) or (exist Exec))
     * \endcode
     *
     * The keys used in the query (Type, ServiceType, Exec) are all
     * fields found in the .desktop files.
     *
     * @param servicetype A service type like 'KMyApp/Plugin' or 'KFilePlugin'.
     * @param constraint  A constraint to limit the choices returned, QString() to
     *                    get all services of the given @p servicetype
     *
     * @return A list of services that satisfy the query
     * @see http://techbase.kde.org/Development/Tutorials/Services/Traders#The_KTrader_Query_Language
     */
    KService::List query(const QString &servicetype, const QString &constraint = QString()) const;

    /**
     * Returns all offers associated with a given servicetype, IGNORING the
     * user preference. The sorting will be the one coming from the InitialPreference
     * in the .desktop files, and services disabled by the user will still be listed here.
     * This is used for "Revert to defaults" buttons in GUIs.
     */
    KService::List defaultOffers(const QString &serviceType, const QString &constraint = QString()) const;
    /**
     * Returns the preferred service for @p serviceType.
     *
     * @param serviceType the service type (e.g. "KMyApp/Plugin")
     * @return the preferred service, or @c nullptr if no service is available
     */
    KService::Ptr preferredService(const QString &serviceType) const;

    /**
     * This is a static pointer to the KServiceTypeTrader singleton.
     *
     * You will need to use this to access the KServiceTypeTrader functionality since the
     * constructors are protected.
     *
     * @return Static KServiceTypeTrader instance
     * @deprecated Since 5.90, see class API docs
     */
    KSERVICE_DEPRECATED_VERSION(5, 90, "See class API docs")
    static KServiceTypeTrader *self();

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 86)
#if KCOREADDONS_ENABLE_DEPRECATED_SINCE(5, 86)
    /**
     * Get a plugin from a trader query
     *
     * Example:
     * \code
     * KMyAppPlugin* plugin = KServiceTypeTrader::createInstanceFromQuery<KMyAppPlugin>( serviceType, QString(), parentObject );
     * if ( plugin ) {
     *     ....
     * }
     * \endcode
     *
     * @param serviceType the type of service for which to find a plugin
     * @param constraint an optional constraint to pass to the trader (see KTrader)
     * @param parent the parent object for the part itself
     * @param args A list of arguments passed to the service component
     * @param error The string passed here will contain an error description.
     * @return A pointer to the newly created object or a null pointer if the
     *         factory was unable to create an object of the given type.
     *
     * @see KPluginFactory::instantiatePlugin
     * @deprecated Since 5.86, Use KPluginMetaData/KPluginFactory or QPluginloader instead
     */
    template<class T>
    KSERVICE_DEPRECATED_VERSION(5, 86, "Use KPluginMetaData/KPluginFactory or QPluginloader instead")
    static T *createInstanceFromQuery(const QString &serviceType,
                                      const QString &constraint = QString(),
                                      QObject *parent = nullptr,
                                      const QVariantList &args = QVariantList(),
                                      QString *error = nullptr)
    {
        return createInstanceFromQuery<T>(serviceType, nullptr, parent, constraint, args, error);
    }
#endif
#endif

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 86)
#if KCOREADDONS_ENABLE_DEPRECATED_SINCE(5, 86)
    /**
     * Get a plugin from a trader query
     *
     * This method works like
     * createInstanceFromQuery(const QString&, const QString&, QObject*, const QVariantList&, QString*),
     * but you can specify an additional parent widget.  This is important for
     * a KPart, for example.
     *
     * @param serviceType the type of service for which to find a plugin
     * @param parentWidget the parent widget for the plugin
     * @param parent the parent object for the part itself
     * @param constraint an optional constraint to pass to the trader (see KTrader)
     * @param args A list of arguments passed to the service component
     * @param error The string passed here will contain an error description.
     * @return A pointer to the newly created object or a null pointer if the
     *         factory was unable to create an object of the given type.
     *
     * @see KPluginFactory::instantiatePlugin
     * @deprecated Since 5.86, Use KPluginMetaData/KPluginFactory or QPluginloader instead
     */
    template<class T>
    KSERVICE_DEPRECATED_VERSION(5, 86, "Use KPluginMetaData/KPluginFactory or QPluginloader instead")
    static T *createInstanceFromQuery(const QString &serviceType,
                                      QWidget *parentWidget,
                                      QObject *parent,
                                      const QString &constraint = QString(),
                                      const QVariantList &args = QVariantList(),
                                      QString *error = nullptr)
    {
        const KService::List offers = self()->query(serviceType, constraint);
        if (error) {
            error->clear();
        }
        for (const KService::Ptr &ptr : offers) {
            T *component = ptr->template createInstance<T>(parentWidget, parent, args, error);
            if (component) {
                return component;
            }
        }
        if (error && error->isEmpty()) {
            *error = QCoreApplication::translate("", "No service matching the requirements was found");
        }
        return nullptr;
    }
#endif
#endif

    /**
     * @internal  (public for KMimeTypeTrader)
     */
    static void applyConstraints(KService::List &lst, const QString &constraint);

private:
    /**
     * @internal
     */
    KServiceTypeTrader();

    // disallow copy ctor and assignment operator
    KServiceTypeTrader(const KServiceTypeTrader &other);
    KServiceTypeTrader &operator=(const KServiceTypeTrader &rhs);

    static KServiceOfferList weightedOffers(const QString &serviceType);

    KServiceTypeTraderPrivate *const d;

    friend class KServiceTypeTraderSingleton;
};

#endif
#endif
