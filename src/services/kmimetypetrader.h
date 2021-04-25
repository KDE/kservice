/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KMIMETYPETRADER_H
#define KMIMETYPETRADER_H

#include <kservice.h>
class KMimeTypeTraderPrivate;
class KServiceOffer;
typedef QList<KServiceOffer> KServiceOfferList;

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 82)
/**
 * @class KMimeTypeTrader kmimetypetrader.h <KMimeTypeTrader>
 *
 * KDE's trader for services associated to a given MIME type.
 *
 * Example: say that you want to the list of all KParts components that can handle HTML.
 * Our code would look like:
 * \code
 * KService::List lst = KMimeTypeTrader::self()->query("text/html",
 *                                                     "KParts/ReadOnlyPart");
 * \endcode
 *
 * If you want to get the preferred KParts component for text/html you would use:
 * @code
 * KService::Ptr service = KMimeTypeTrader::self()->preferredService("text/html",
 *                                                                   "KParts/ReadOnlyPart");
 * @endcode
 * Although if this is about loading that component you would use createPartInstanceFromQuery() directly.
 *
 * @see KServiceTypeTrader, KService
 *
 * @deprecated since 5.82. For querying applications use KApplicationTrader.
 * For querying KParts use KParts::PartLoader.
 * For querying plugins use KPluginLoader.
 */
class KSERVICE_EXPORT KMimeTypeTrader
{
public:
    /**
     * Standard destructor
     */
    ~KMimeTypeTrader();

    /**
     * This method returns a list of services which are associated with a given MIME type.
     *
     * Example usage:
     * To get list of applications that can handle a given MIME type,
     * set @p genericServiceType to "Application" (which is the default).
     * To get list of embeddable components that can handle a given MIME type,
     * set @p genericServiceType to "KParts/ReadOnlyPart".
     *
     * The constraint parameter is used to limit the possible choices
     * returned based on the constraints you give it.
     *
     * The @p constraint language is rather full.  The most common
     * keywords are AND, OR, NOT, IN, and EXIST, all used in an
     * almost spoken-word form.  An example is:
     * \code
     * (Type == 'Service') and (('Browser/View' in ServiceTypes) and (exist Library))
     * \endcode
     *
     * The keys used in the query (Type, ServiceTypes, Library) are all
     * fields found in the .desktop files.
     *
     * @param mimeType a MIME type like 'text/plain' or 'text/html'
     * @param genericServiceType a basic service type, like 'KParts/ReadOnlyPart' or 'Application'
     * @param constraint  A constraint to limit the choices returned, QString() to
     *                    get all services that can handle the given @p mimetype
     *
     * @return A list of services that satisfy the query, sorted by preference
     * (preferred service first)
     * @see http://techbase.kde.org/Development/Tutorials/Services/Traders#The_KTrader_Query_Language
     * @deprecated since 5.82. For querying applications use KApplicationTrader::query().
     * For querying KParts use KParts::PartLoader::partsForMimeType().
     * For querying plugins use KPluginLoader.
     */
    KSERVICE_DEPRECATED_VERSION(5, 82, "See API docs.")
    KService::List
    query(const QString &mimeType, const QString &genericServiceType = QStringLiteral("Application"), const QString &constraint = QString()) const;

    /**
     * Returns the preferred service for @p mimeType and @p genericServiceType
     *
     * This is almost like query().first(), except that it also checks
     * if the service is allowed as a preferred service (see KService::allowAsDefault).
     *
     * @param mimeType the MIME type (see query())
     * @param genericServiceType the service type (see query())
     * @return the preferred service, or @c nullptr if no service is available.
     * @deprecated since 5.82. For querying applications use KApplicationTrader::preferredService(). For querying KParts use
     * KParts::PartLoader::partsForMimeType().first().
     */
    KSERVICE_DEPRECATED_VERSION(5, 82, "See API docs.")
    KService::Ptr preferredService(const QString &mimeType, const QString &genericServiceType = QStringLiteral("Application"));

    /**
     * This method creates and returns a part object from the trader query for a given \p mimeType.
     *
     * Example:
     * \code
     * KParts::ReadOnlyPart* part = KMimeTypeTrader::createPartInstanceFromQuery<KParts::ReadOnlyPart>("text/plain", parentWidget, parentObject);
     * if (part) {
     *     part->openUrl(url);
     *     part->widget()->show();  // also insert the widget into a layout
     * }
     * \endcode
     *
     * @param mimeType the MIME type which this part is associated with
     * @param parentWidget the parent widget, will be set as the parent of the part's widget
     * @param parent the parent object for the part itself
     * @param constraint an optional constraint to pass to the trader
     * @param args A list of arguments passed to the service component
     * @param error The string passed here will contain an error description.
     * @return A pointer to the newly created object or a null pointer if the
     *         factory was unable to create an object of the given type.
     * @deprecated since 5.82. For KParts use KParts::PartLoader::createPartInstanceForMimeType().
     * Otherwise use KPluginLoader.
     */
    template<class T>
    KSERVICE_DEPRECATED_VERSION(5, 82, "See API docs.")
    static T *createPartInstanceFromQuery(const QString &mimeType,
                                          QWidget *parentWidget = nullptr,
                                          QObject *parent = nullptr,
                                          const QString &constraint = QString(),
                                          const QVariantList &args = QVariantList(),
                                          QString *error = nullptr)
    {
        const KService::List offers = self()->query(mimeType, QStringLiteral("KParts/ReadOnlyPart"), constraint);
        for (const KService::Ptr &ptr : offers) {
            T *component = ptr->template createInstance<T>(parentWidget, parent, args, error);
            if (component) {
                if (error) {
                    error->clear();
                }
                return component;
            }
        }
        if (error) {
            *error = QCoreApplication::translate("", "No service matching the requirements was found");
        }
        return nullptr;
    }

    /**
     * This can be used to create a service instance from a MIME type query.
     *
     * @param mimeType a MIME type like 'text/plain' or 'text/html'
     * @param serviceType a basic service type
     * @param parent the parent object for the plugin itself
     * @param constraint  A constraint to limit the choices returned, QString() to
     *                    get all services that can handle the given @p mimeType
     * @param args A list of arguments passed to the service component
     * @param error The string passed here will contain an error description.
     * @return A pointer to the newly created object or a null pointer if the
     *         factory was unable to create an object of the given type.
     * @deprecated since 5.82. For KParts use KParts::PartLoader::createPartInstanceForMimeType().
     * Otherwise use KPluginLoader.
     */
    template<class T>
    KSERVICE_DEPRECATED_VERSION(5, 82, "See API docs.")
    static T *createInstanceFromQuery(const QString &mimeType,
                                      const QString &serviceType,
                                      QObject *parent = nullptr,
                                      const QString &constraint = QString(),
                                      const QVariantList &args = QVariantList(),
                                      QString *error = nullptr)
    {
        const KService::List offers = self()->query(mimeType, serviceType, constraint);
        for (const KService::Ptr &ptr : offers) {
            T *component = ptr->template createInstance<T>(parent, args, error);
            if (component) {
                if (error) {
                    error->clear();
                }
                return component;
            }
        }
        if (error) {
            *error = QCoreApplication::translate("", "No service matching the requirements was found");
        }
        return nullptr;
    }

    /**
     * This is a static pointer to the KMimeTypeTrader singleton.
     *
     * You will need to use this to access the KMimeTypeTrader functionality since the
     * constructors are protected.
     *
     * @return Static KMimeTypeTrader instance
     */
    static KMimeTypeTrader *self();

private:
    /**
     * @internal
     */
    KMimeTypeTrader();

private:
    KMimeTypeTraderPrivate *const d;

    // class-static so that it can access KSycocaEntry::offset()
    static void filterMimeTypeOffers(KServiceOfferList &list, const QString &genericServiceType);
    static void filterMimeTypeOffers(KService::List &list, const QString &genericServiceType);
    friend class KMimeTypeTraderSingleton;
};

#endif

#endif /* KMIMETYPETRADER_H */
