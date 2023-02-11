/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kservicetypefactory_p.h"
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
}

QMetaType::Type KServiceTypeFactory::findPropertyTypeByName(const QString &_name)
{
    if (!sycocaDict()) {
        return QMetaType::UnknownType; // Error!
    }

    assert(!sycoca()->isBuilding());

    return static_cast<QMetaType::Type>(m_propertyTypeDict.value(_name, QMetaType::UnknownType));
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
