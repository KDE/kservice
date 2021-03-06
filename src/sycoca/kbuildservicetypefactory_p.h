/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KBUILD_SERVICE_TYPE_FACTORY_H
#define KBUILD_SERVICE_TYPE_FACTORY_H

#include <QStringList>
#include <kservicetypefactory_p.h>

/**
 * Service-type factory for building ksycoca
 * @internal
 */
class KBuildServiceTypeFactory : public KServiceTypeFactory
{
public:
    /**
     * Create factory
     */
    explicit KBuildServiceTypeFactory(KSycoca *db);

    ~KBuildServiceTypeFactory() override;

    /**
     * Find a service type in the database file
     * @return a pointer to the servicetype in the memory dict (don't free!)
     */
    KServiceType::Ptr findServiceTypeByName(const QString &_name) override;

    /**
     * Construct a KServiceType from a config file.
     */
    KSycocaEntry *createEntry(const QString &file) const override;

    KServiceType *createEntry(int) const override
    {
        assert(0);
        return nullptr;
    }

    /**
     * Add entry
     */
    void addEntry(const KSycocaEntry::Ptr &newEntry) override;

    /**
     * Write out service type specific index files.
     */
    void save(QDataStream &str) override;

    /**
     * Write out header information
     *
     * Don't forget to call the parent first when you override
     * this function.
     */
    void saveHeader(QDataStream &str) override;
};

#endif
