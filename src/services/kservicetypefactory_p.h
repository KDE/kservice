/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSERVICETYPEFACTORY_P_H
#define KSERVICETYPEFACTORY_P_H

#include <assert.h>

#include <QStringList>

#include "kservicetype.h"
#include "ksycocafactory_p.h"

class KSycoca;

class KServiceType;

/**
 * @internal
 * A sycoca factory for service types
 * It loads the service types from parsing directories (e.g. servicetypes/)
 * but can also create service types from data streams or single config files
 * @see KServiceType
 */
class KServiceTypeFactory : public KSycocaFactory
{
    K_SYCOCAFACTORY(KST_KServiceTypeFactory)
public:
    /**
     * Create factory
     */
    explicit KServiceTypeFactory(KSycoca *db);

    ~KServiceTypeFactory() override;

    /**
     * Not meant to be called at this level
     */
    KSycocaEntry *createEntry(const QString &) const override
    {
        assert(0);
        return nullptr;
    }

    /**
     * Find a service type in the database file (allocates it)
     * Overloaded by KBuildServiceTypeFactory to return a memory one.
     */
    virtual KServiceType::Ptr findServiceTypeByName(const QString &_name);

    /**
     * Find a the property type of a named property.
     */
    QMetaType::Type findPropertyTypeByName(const QString &_name);

    /**
     * @return all servicetypes
     * Slow and memory consuming, avoid using
     */
    KServiceType::List allServiceTypes();

    /**
     * Returns the directories to watch for this factory.
     */
    static QStringList resourceDirs();

    /**
     * @return the unique servicetype factory, creating it if necessary
     */
    static KServiceTypeFactory *self();

protected:
    KServiceType *createEntry(int offset) const override;

    // protected for KBuildServiceTypeFactory
    QMap<QString, int> m_propertyTypeDict;

protected:
    void virtual_hook(int id, void *data) override;

private:
    class KServiceTypeFactoryPrivate *d = nullptr;
};

#endif
