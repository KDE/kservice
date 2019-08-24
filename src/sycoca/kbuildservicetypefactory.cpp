/*  This file is part of the KDE libraries
 *  Copyright (C) 1999 David Faure   <faure@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation;
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include "kbuildservicetypefactory_p.h"
#include "ksycoca.h"
#include "ksycocadict_p.h"
#include "ksycocaresourcelist_p.h"
#include "sycocadebug.h"

#include <QDebug>
#include <assert.h>
#include <kdesktopfile.h>
#include <kconfiggroup.h>
#include <QHash>
#include <qstandardpaths.h>

KBuildServiceTypeFactory::KBuildServiceTypeFactory(KSycoca *db)
    : KServiceTypeFactory(db)
{
    m_resourceList = new KSycocaResourceList;
    m_resourceList->add("servicetypes", QStringLiteral("kservicetypes5"), QStringLiteral("*.desktop"));
}

KBuildServiceTypeFactory::~KBuildServiceTypeFactory()
{
    delete m_resourceList;
}

KServiceType::Ptr KBuildServiceTypeFactory::findServiceTypeByName(const QString &_name)
{
    assert(sycoca()->isBuilding());
    // We're building a database - the service type must be in memory
    KSycocaEntry::Ptr servType = m_entryDict->value(_name);
    return KServiceType::Ptr(static_cast<KServiceType*>(servType.data()));
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

    KDesktopFile desktopFile(QStandardPaths::GenericDataLocation, QLatin1String("kservicetypes5/") + file);
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

void
KBuildServiceTypeFactory::saveHeader(QDataStream &str)
{
    KSycocaFactory::saveHeader(str);
    str << qint32(m_propertyTypeDict.count());
    for (QMap<QString, int>::ConstIterator it = m_propertyTypeDict.constBegin(); it != m_propertyTypeDict.constEnd(); ++it) {
        str << it.key() << qint32(it.value());
    }
}

void
KBuildServiceTypeFactory::save(QDataStream &str)
{
    KSycocaFactory::save(str);
#if 0 // not needed since don't have any additional index anymore
    int endOfFactoryData = str.device()->pos();

    // Update header (pass #3)
    saveHeader(str);

    // Seek to end.
    str.device()->seek(endOfFactoryData);
#endif
}

void
KBuildServiceTypeFactory::addEntry(const KSycocaEntry::Ptr &newEntry)
{
    KSycocaFactory::addEntry(newEntry);

    KServiceType::Ptr serviceType(static_cast<KServiceType*>(newEntry.data()));

    const QMap<QString, QVariant::Type> &pd = serviceType->propertyDefs();
    QMap<QString, QVariant::Type>::ConstIterator pit = pd.begin();
    for (; pit != pd.end(); ++pit) {
        const QString property = pit.key();
        QMap<QString, int>::iterator dictit = m_propertyTypeDict.find(property);
        if (dictit == m_propertyTypeDict.end()) {
            m_propertyTypeDict.insert(property, pit.value());
        } else if (*dictit != static_cast<int>(pit.value())) {
            qCWarning(SYCOCA) << "Property '" << property << "' is defined multiple times (" << serviceType->name() << ")";
        }
    }
}

