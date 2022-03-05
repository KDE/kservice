/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kbuildservicetypefactory_p.h"
#include "ksycoca.h"
#include "ksycocadict_p.h"
#include "ksycocaresourcelist_p.h"
#include "sycocadebug.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <QDebug>
#include <QFileInfo>
#include <QHash>
#include <QStandardPaths>
#include <assert.h>

KBuildServiceTypeFactory::KBuildServiceTypeFactory(KSycoca *db)
    : KServiceTypeFactory(db)
{
    m_resourceList.emplace_back("servicetypes", QStringLiteral("kservicetypes5"), QStringLiteral("*.desktop"));
}

KBuildServiceTypeFactory::~KBuildServiceTypeFactory()
{
}

KServiceType::Ptr KBuildServiceTypeFactory::findServiceTypeByName(const QString &_name)
{
    assert(sycoca()->isBuilding());
    // We're building a database - the service type must be in memory
    KSycocaEntry::Ptr servType = m_entryDict->value(_name);
    return KServiceType::Ptr(static_cast<KServiceType *>(servType.data()));
}

KSycocaEntry *KBuildServiceTypeFactory::createEntry(const QString &file) const
{
    QString name = file;
    int pos = name.lastIndexOf(QLatin1Char('/'));
    if (pos != -1) {
        name.remove(0, pos + 1);
    }

    if (name.isEmpty()) {
        return nullptr;
    }

    QString filePath = QLatin1String("kservicetypes5/") + file;
    const QString qrcFilePath = QLatin1String(":/") + filePath;
    if (QFileInfo::exists(qrcFilePath)) {
        filePath = qrcFilePath;
    }
    KDesktopFile desktopFile(QStandardPaths::GenericDataLocation, filePath);
    const KConfigGroup desktopGroup = desktopFile.desktopGroup();

    if (desktopGroup.readEntry("Hidden", false) == true) {
        return nullptr;
    }

    const QString type = desktopGroup.readEntry("Type");
    if (type != QLatin1String("ServiceType")) {
        qCWarning(SYCOCA) << "The service type config file " << desktopFile.fileName() << " has Type=" << type << " instead of Type=ServiceType";
        return nullptr;
    }

    const QString serviceType = desktopGroup.readEntry("X-KDE-ServiceType");

    if (serviceType.isEmpty()) {
        qCWarning(SYCOCA) << "The service type config file " << desktopFile.fileName() << " does not contain a ServiceType=... entry";
        return nullptr;
    }

    KServiceType *e = new KServiceType(&desktopFile);

    if (e->isDeleted()) {
        delete e;
        return nullptr;
    }

    if (!(e->isValid())) {
        qCWarning(SYCOCA) << "Invalid ServiceType : " << file;
        delete e;
        return nullptr;
    }

    return e;
}

void KBuildServiceTypeFactory::saveHeader(QDataStream &str)
{
    KSycocaFactory::saveHeader(str);
    str << qint32(m_propertyTypeDict.count());
    for (auto it = m_propertyTypeDict.cbegin(); it != m_propertyTypeDict.cend(); ++it) {
        str << it.key() << static_cast<qint32>(it.value());
    }
}

void KBuildServiceTypeFactory::save(QDataStream &str)
{
    KSycocaFactory::save(str);
}

void KBuildServiceTypeFactory::addEntry(const KSycocaEntry::Ptr &newEntry)
{
    KSycocaFactory::addEntry(newEntry);

    KServiceType::Ptr serviceType(static_cast<KServiceType *>(newEntry.data()));

    const QMap<QString, QVariant::Type> &pd = serviceType->propertyDefs();
    for (auto pit = pd.cbegin(); pit != pd.cend(); ++pit) {
        const QString &property = pit.key();
        const QVariant::Type &type = pit.value();
        auto dictit = m_propertyTypeDict.constFind(property);
        if (dictit == m_propertyTypeDict.cend()) {
            m_propertyTypeDict.insert(property, type);
        } else if (dictit.value() != static_cast<int>(type)) {
            qCWarning(SYCOCA) << "Property '" << property << "' is defined multiple times (" << serviceType->name() << ")";
        }
    }
}
