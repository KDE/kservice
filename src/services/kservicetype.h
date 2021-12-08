/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __kservicetype_h__
#define __kservicetype_h__

#include <ksycocaentry.h>

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <KConfig>

class KDesktopFile;
class KServiceTypePrivate;

/**
 * @class KServiceType kservicetype.h <KServiceType>
 *
 * A service type is, well, a type of service, where a service is an application or plugin.
 * For instance, "KOfficeFilter", which is the type of all koffice filters, is a service type.
 * In order to discover services of a given type, using KServiceTypeTrader.
 * Service types are stored as desktop files in $KDEDIR/share/servicetypes.
 * @see KService, KServiceTypeTrader
 * @deprecated Since  5.90, this class is an implementation detail of KService.
 * Use @ref KService or @ref KApplicationTrader instead.
 */
class KSERVICE_EXPORT KServiceType : public KSycocaEntry // TODO KDE5: inherit kshared, but move KSycocaEntry to KServiceTypePrivate
{
public:
    /**
     * A shared data pointer for KServiceType.
     */
    typedef QExplicitlySharedDataPointer<KServiceType> Ptr;
    /**
     * A list of shared data pointers for KServiceType.
     */
    typedef QList<Ptr> List;

    /**
     * Construct a service type and take all information from a desktop file.
     * @param config the configuration file
     * @deprecated Since 5.90, see class API docs
     */
    KSERVICE_DEPRECATED_VERSION(5, 90, "See class API docs")
    explicit KServiceType(KDesktopFile *config);

    ~KServiceType() override;

    /**
     * Returns the descriptive comment associated, if any.
     * @return the comment, or QString()
     */
    QString comment() const;

    /**
     * Checks whether this service type inherits another one.
     * @return true if this service type inherits another one
     * @see parentServiceType()
     */
    bool isDerived() const;

    /**
     * If this service type inherits from another service type,
     * return the name of the parent.
     * @return the parent service type, or QString:: null if not set
     * @see isDerived()
     */
    QString parentServiceType() const;

    /**
     * Checks whether this service type is or inherits from @p servTypeName.
     * @return true if this servicetype is or inherits from @p servTypeName
     */
    bool inherits(const QString &servTypeName) const;

    /**
     * Returns the type of the property definition with the given @p _name.
     *
     * @param _name the name of the property
     * @return the property type, or null if not found
     * @see propertyDefNames
     */
    QVariant::Type propertyDef(const QString &_name) const;

    /**
     * Returns the list of all property definitions for this servicetype.
     * Those are properties of the services implementing this servicetype.
     * For instance,
     * @code
     * [PropertyDef::X-KDevelop-Version]
     * Type=int
     * @endcode
     * means that all kdevelop plugins have in their .desktop file a line like
     * @code
     * X-KDevelop-Version=<some value>
     * @endcode
     */
    QStringList propertyDefNames() const;

    /// @internal (for KBuildServiceTypeFactory)
    QMap<QString, QVariant::Type> propertyDefs() const;

    /**
     * @internal
     * Pointer to parent service type
     */
    Ptr parentType();
    /**
     * @internal  only used by kbuildsycoca
     * Register offset into offers list
     */
    void setServiceOffersOffset(int offset);
    /**
     * @internal
     */
    int serviceOffersOffset() const;

    /**
     * Returns a pointer to the servicetype '_name' or @c nullptr if the
     *         service type is unknown.
     * VERY IMPORTANT : don't store the result in a KServiceType * !
     * @param _name the name of the service type to search
     * @return the pointer to the service type, or @c nullptr
     */
    static Ptr serviceType(const QString &_name);

    /**
     * Returns a list of all the supported servicetypes. Useful for
     *         showing the list of available servicetypes in a listbox,
     *         for example.
     * More memory consuming than the ones above, don't use unless
     * really necessary.
     * @return the list of all services
     */
    static List allServiceTypes();

private:
    friend class KServiceTypeFactory;
    /**
     * @internal construct a service from a stream.
     * The stream must already be positioned at the correct offset
     */
    KServiceType(QDataStream &_str, int offset);

    Q_DECLARE_PRIVATE(KServiceType)
};

// QDataStream& operator>>( QDataStream& _str, KServiceType& s );
// QDataStream& operator<<( QDataStream& _str, KServiceType& s );

#endif
