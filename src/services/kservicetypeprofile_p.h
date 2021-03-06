/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSERVICETYPEPROFILE_P_H
#define KSERVICETYPEPROFILE_P_H

#include <QMap>

/**
 * @internal
 */
class KServiceTypeProfileEntry
{
public:
    explicit KServiceTypeProfileEntry()
    {
    }

    /**
     * Add a service to this profile.
     * @param _service the name of the service
     * @param _preference the user's preference value, must be positive,
     *              bigger is better
     * @param _allow_as_default true if the service should be used as
     *                 default
     */
    void addService(const QString &service, int preference = 1)
    {
        m_mapServices.insert(service, preference);
    }

    /**
     * Map of all services for which we have assessments.
     * Key: service ID
     * Value: preference
     */
    QMap<QString, int> m_mapServices;
};

#endif /* KSERVICETYPEPROFILE_P_H */
