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

    /// @internal (for KBuildServiceTypeFactory)
    QMap<QString, QMetaType::Type> propertyDefs() const;

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
