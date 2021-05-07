/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __kservicetypeprofile_h__
#define __kservicetypeprofile_h__

#include <QString>

#include <kservicetypetrader.h>

#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 66)
/**
 * KServiceTypeProfile represents the user's preferences for services
 * of a service type.
 * It consists of a list of services (service offers) for the service type
 * that is sorted by the user's preference.
 * KServiceTypeTrader uses KServiceTypeProfile to
 * get results sorted according to the user's preference.
 *
 * @see KService
 * @see KServiceType
 * @see KServiceTypeTrader
 * @short Represents the user's preferences for services of a service type
 */
namespace KServiceTypeProfile
{
/**
 * Write the complete profile for a given servicetype.
 * Do not use this for MIME types.
 * @param serviceType The name of the servicetype.
 * @param services Ordered list of services, from the preferred one to the least preferred one.
 * @param disabledServices List of services which are normally associated with this serviceType,
 * but which should be disabled, i.e. trader queries will not return them.
 * @deprecated since 5.66, unused.
 */
KSERVICE_EXPORT
KSERVICE_DEPRECATED_VERSION(5, 66, "Unused")
void writeServiceTypeProfile(const QString &serviceType, const KService::List &services, const KService::List &disabledServices = KService::List());

/**
 * Delete the complete profile for a given servicetype, reverting to the default
 * preference order (the one specified by InitialPreference in the .desktop files).
 *
 * Do not use this for MIME types.
 * @param serviceType The name of the servicetype.
 */
KSERVICE_EXPORT
KSERVICE_DEPRECATED_VERSION(5, 66, "Unused")
void deleteServiceTypeProfile(const QString &serviceType);
}
#endif

namespace KServiceTypeProfile
{
/**
 * @internal, for KServiceTypeTrader & unit test
 */
KSERVICE_EXPORT bool hasProfile(const QString &serviceType);

/**
 * Clear all cached information
 * @internal, for KServiceTypeFactory
 */
void clearCache();

}

#endif
