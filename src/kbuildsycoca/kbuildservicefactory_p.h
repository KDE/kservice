/* This file is part of the KDE project
   Copyright (C) 1999, 2007 David Faure <faure@kde.org>
                 1999 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KBUILD_SERVICE_FACTORY_H
#define KBUILD_SERVICE_FACTORY_H

#include <QtCore/QStringList>

#include "kmimeassociations_p.h"
#include <kservicefactory_p.h>
// We export the services to the service group factory!
class KBuildServiceGroupFactory;
class KBuildMimeTypeFactory;
class KServiceTypeFactory;

/**
 * Service factory for building ksycoca
 * @internal
 */
class KBuildServiceFactory : public KServiceFactory
{
public:
    /**
     * Create factory
     */
    KBuildServiceFactory(KServiceTypeFactory *serviceTypeFactory,
                         KBuildMimeTypeFactory *mimeTypeFactory,
                         KBuildServiceGroupFactory *serviceGroupFactory);

    virtual ~KBuildServiceFactory();

    /// Reimplemented from KServiceFactory
    KService::Ptr findServiceByDesktopName(const QString &name) Q_DECL_OVERRIDE;
    /// Reimplemented from KServiceFactory
    KService::Ptr findServiceByDesktopPath(const QString &name) Q_DECL_OVERRIDE;
    /// Reimplemented from KServiceFactory
    KService::Ptr findServiceByMenuId(const QString &menuId) Q_DECL_OVERRIDE;

    /**
     * Construct a KService from a config file.
     */
    KSycocaEntry *createEntry(const QString &file) const Q_DECL_OVERRIDE;

    KService *createEntry(int) const Q_DECL_OVERRIDE
    {
        assert(0);
        return 0;
    }

    /**
     * Add a new entry.
     */
    void addEntry(const KSycocaEntry::Ptr &newEntry) Q_DECL_OVERRIDE;

    /**
     * Write out service specific index files.
     */
    void save(QDataStream &str) Q_DECL_OVERRIDE;

    /**
     * Write out header information
     *
     * Don't forget to call the parent first when you override
     * this function.
     */
    void saveHeader(QDataStream &str) Q_DECL_OVERRIDE;

    void postProcessServices();

private:
    void populateServiceTypes();
    void saveOfferList(QDataStream &str);
    void collectInheritedServices();
    void collectInheritedServices(const QString &mime, QSet<QString> &visitedMimes);

    QHash<QString, KService::Ptr> m_nameMemoryHash; // m_nameDict is not useable while building ksycoca
    QHash<QString, KService::Ptr> m_relNameMemoryHash; // m_relNameDict is not useable while building ksycoca
    QHash<QString, KService::Ptr> m_menuIdMemoryHash; // m_menuIdDict is not useable while building ksycoca
    QSet<KSycocaEntry::Ptr> m_dupeDict;

    KOfferHash m_offerHash;

    KServiceTypeFactory *m_serviceTypeFactory;
    KBuildMimeTypeFactory *m_mimeTypeFactory;
    KBuildServiceGroupFactory *m_serviceGroupFactory;
};

#endif
