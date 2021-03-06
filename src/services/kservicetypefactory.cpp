/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kservicetypefactory_p.h"
#include "kservicetypeprofile.h"
#include "ksycoca.h"
#include "ksycocadict_p.h"
#include "ksycocatype.h"
#include "ksycocautils_p.h"
#include "servicesdebug.h"

#include <assert.h>

KServiceTypeFactory::KServiceTypeFactory(KSycoca *db)
    : KSycocaFactory(KST_KServiceTypeFactory, db)
{
    if (!sycoca()->isBuilding()) {
        QDataStream *str = stream();
        if (str) {
            // Read Header
            qint32 n;
            (*str) >> n;
            if (n > 1024) {
                KSycoca::flagError();
            } else {
                QString string;
                qint32 i;
                for (; n; --n) {
                    *str >> string >> i;
                    m_propertyTypeDict.insert(string, i);
                }
            }
        } else {
            qWarning() << "Could not open sycoca database, you must run kbuildsycoca first!";
        }
    }
}

KServiceTypeFactory::~KServiceTypeFactory()
{
    if (!sycoca()->isBuilding()) {
        KServiceTypeProfile::clearCache();
    }
}

KServiceType::Ptr KServiceTypeFactory::findServiceTypeByName(const QString &_name)
{
    if (!sycocaDict()) {
        return KServiceType::Ptr(); // Error!
    }
    assert(!sycoca()->isBuilding());
    int offset = sycocaDict()->find_string(_name);
    if (!offset) {
        return KServiceType::Ptr(); // Not found
    }
    KServiceType::Ptr newServiceType(createEntry(offset));

    // Check whether the dictionary was right.
    if (newServiceType && (newServiceType->name() != _name)) {
        // No it wasn't...
        newServiceType = nullptr; // Not found
    }
    return newServiceType;
}

QVariant::Type KServiceTypeFactory::findPropertyTypeByName(const QString &_name)
{
    if (!sycocaDict()) {
        return QVariant::Invalid; // Error!
    }

    assert(!sycoca()->isBuilding());

    return static_cast<QVariant::Type>(m_propertyTypeDict.value(_name, QVariant::Invalid));
}

KServiceType::List KServiceTypeFactory::allServiceTypes()
{
    KServiceType::List result;
    const KSycocaEntry::List list = allEntries();
    for (KSycocaEntry::List::ConstIterator it = list.begin(); it != list.end(); ++it) {
        if ((*it)->isType(KST_KServiceType)) {
            KServiceType::Ptr newServiceType(static_cast<KServiceType *>((*it).data()));
            result.append(newServiceType);
        }
    }
    return result;
}

QStringList KServiceTypeFactory::resourceDirs()
{
    return KSycocaFactory::allDirectories(QStringLiteral("kservicetypes5"));
}

KServiceType *KServiceTypeFactory::createEntry(int offset) const
{
    KSycocaType type;
    QDataStream *str = sycoca()->findEntry(offset, type);
    if (!str) {
        return nullptr;
    }

    if (type != KST_KServiceType) {
        qCWarning(SERVICES) << "KServiceTypeFactory: unexpected object entry in KSycoca database (type=" << int(type) << ")";
        return nullptr;
    }

    KServiceType *newEntry = new KServiceType(*str, offset);
    if (newEntry && !newEntry->isValid()) {
        qCWarning(SERVICES) << "KServiceTypeFactory: corrupt object in KSycoca database!";
        delete newEntry;
        newEntry = nullptr;
    }
    return newEntry;
}

void KServiceTypeFactory::virtual_hook(int id, void *data)
{
    KSycocaFactory::virtual_hook(id, data);
}
