/* This file is part of the KDE project
   Copyright (C) 2000 Waldo Bastian <bastian@kde.org>

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

#ifndef KSERVICEGROUPFACTORY_P_H
#define KSERVICEGROUPFACTORY_P_H


#include "kservicegroup.h"
#include "ksycocafactory_p.h"
#include <assert.h>

class KSycoca;
class KSycocaDict;

/**
 * @internal
 * A sycoca factory for service groups (e.g. list of applications)
 * It loads the services from parsing directories (e.g. share/applications/)
 *
 * Exported for kbuildsycoca, but not installed.
 */
class KServiceGroupFactory : public KSycocaFactory
{
    K_SYCOCAFACTORY(KST_KServiceGroupFactory)
public:
    /**
     * Create factory
     */
    explicit KServiceGroupFactory(KSycoca *db);
    ~KServiceGroupFactory() override;

    /**
     * Construct a KServiceGroup from a config file.
     */
    KSycocaEntry *createEntry(const QString &) const override
    {
        assert(0);
        return nullptr;
    }

    /**
     * Find a group ( by desktop path, e.g. "Applications/Editors")
     */
    virtual KServiceGroup::Ptr findGroupByDesktopPath(const QString &_name, bool deep = true);

    /**
     * Find a base group by name, e.g. "settings"
     */
    KServiceGroup::Ptr findBaseGroup(const QString &_baseGroupName, bool deep = true);

    /**
     * @return the unique service group factory, creating it if necessary
     */
    static KServiceGroupFactory *self();
protected:
    KServiceGroup *createGroup(int offset, bool deep) const;
    KServiceGroup *createEntry(int offset) const override;
    KSycocaDict *m_baseGroupDict;
    int m_baseGroupDictOffset;

protected:
    void virtual_hook(int id, void *data) override;
private:
    class KServiceGroupFactoryPrivate *d;
};

#endif
