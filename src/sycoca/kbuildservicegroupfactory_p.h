/* This file is part of the KDE project
   Copyright (C) 2000 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KBUILD_SERVICE_GROUP_FACTORY_H
#define KBUILD_SERVICE_GROUP_FACTORY_H

#include <kservice.h>
#include <kservicegroupfactory_p.h>
#include <QStringList>

/**
 * Service group factory for building ksycoca
 * @internal
 */
class KBuildServiceGroupFactory : public KServiceGroupFactory
{
public:
    /**
     * Create factory
     */
    explicit KBuildServiceGroupFactory(KSycoca *db);

    ~KBuildServiceGroupFactory() override;

    /**
     * Create new entry.
     */
    KServiceGroup *createEntry(const QString &) const override;

    KServiceGroup *createEntry(int) const override
    {
        assert(0);
        return nullptr;
    }

    /**
     * Adds the entry @p newEntry to the menu @p menuName
     */
    void addNewEntryTo(const QString &menuName, const KService::Ptr &newEntry);

    /**
     * Adds the entry @p newEntry to the "parent group" @p parent, creating
     * the group if necassery.
     * A "parent group" is a group of services that all have the same
     * "X-KDE-ParentApp".
     */
    void addNewChild(const QString &parent, const KSycocaEntry::Ptr &newEntry);

    /**
     * Add new menu @p menuName defined by @p file
     * When @p entry is non-null it is re-used, otherwise a new group is created.
     * A pointer to the group is returned.
     */
    KServiceGroup::Ptr addNew(const QString &menuName, const QString &file, KServiceGroup::Ptr entry, bool isDeleted);

    /**
     * Find a group ( by desktop path, e.g. "Applications/Editors")
     */
    KServiceGroup::Ptr findGroupByDesktopPath(const QString &_name, bool deep = true) override;

    /**
     * Add a new menu entry
     */
    void addEntry(const KSycocaEntry::Ptr &newEntry) override;

    /**
     * Write out servicegroup  specific index files.
     */
    void save(QDataStream &str) override;

    /**
     * Write out header information
     */
    void saveHeader(QDataStream &str) override;
};

#endif
