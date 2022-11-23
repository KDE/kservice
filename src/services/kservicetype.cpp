/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kservicetype.h"
#include "kservice.h"
#include "kservicefactory_p.h"
#include "kservicetype_p.h"
#include "kservicetypefactory_p.h"
#include "kservicetypeprofile.h"
#include "ksycoca.h"
#include "ksycoca_p.h"
#include "servicesdebug.h"
#include <KConfigGroup>
#include <KDesktopFile>
#include <assert.h>

extern int servicesDebugArea();

template QDataStream &operator>><QString, QVariant>(QDataStream &, QMap<QString, QVariant> &);
template QDataStream &operator<< <QString, QVariant>(QDataStream &, const QMap<QString, QVariant> &);

KServiceType::KServiceType(KDesktopFile *config)
    : KSycocaEntry(*new KServiceTypePrivate(config->fileName()))
{
    Q_D(KServiceType);
    d->init(config);
}

void KServiceTypePrivate::init(KDesktopFile *config)
{
    //    Q_Q(KServiceType);

    KConfigGroup desktopGroup = config->desktopGroup();
    m_strName = desktopGroup.readEntry("X-KDE-ServiceType");
    m_strComment = desktopGroup.readEntry("Comment");
    deleted = desktopGroup.readEntry("Hidden", false);

    // We store this as property to preserve BC, we can't change that
    // because KSycoca needs to remain BC between KDE 2.x and KDE 3.x
    QString sDerived = desktopGroup.readEntry("X-KDE-Derived");
    m_bDerived = !sDerived.isEmpty();
    if (m_bDerived) {
        m_mapProps.insert(QStringLiteral("X-KDE-Derived"), sDerived);
    }

    const QStringList lst = config->groupList();

    for (const auto &groupName : lst) {
        if (QLatin1String marker("Property::"); groupName.startsWith(marker)) {
            KConfigGroup cg(config, groupName);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QVariant variant = QVariant::nameToType(cg.readEntry("Type").toLatin1().constData());
#else
            QVariant variant(QMetaType::fromName(cg.readEntry("Type").toLatin1().constData()));
#endif
            variant = cg.readEntry("Value", variant);

            if (variant.isValid()) {
                m_mapProps.insert(groupName.mid(marker.size()), variant);
            }
        } else if (QLatin1String marker("PropertyDef::"); groupName.startsWith(marker)) {
            KConfigGroup cg(config, groupName);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            m_mapPropDefs.insert(groupName.mid(marker.size()), QVariant::nameToType(cg.readEntry("Type").toLatin1().constData()));
#else
            m_mapPropDefs.insert(groupName.mid(marker.size()),
                                 static_cast<QVariant::Type>(QMetaType::fromName(cg.readEntry("Type").toLatin1().constData()).id()));
#endif
        }
    }
}

KServiceType::KServiceType(QDataStream &_str, int offset)
    : KSycocaEntry(*new KServiceTypePrivate(_str, offset))
{
    Q_D(KServiceType);
    d->load(_str);
}

void KServiceTypePrivate::load(QDataStream &_str)
{
    qint8 b;
    QString dummy;
    _str >> m_strName >> dummy >> m_strComment >> m_mapProps >> m_mapPropDefs >> b >> m_serviceOffersOffset;
    m_bDerived = m_mapProps.contains(QLatin1String("X-KDE-Derived"));
}

void KServiceTypePrivate::save(QDataStream &_str)
{
    KSycocaEntryPrivate::save(_str);
    // !! This data structure should remain binary compatible at all times !!
    // You may add new fields at the end. Make sure to update the version
    // number in ksycoca.h
    _str << m_strName << QString() /*was icon*/ << m_strComment << m_mapProps << m_mapPropDefs << qint8(1) << m_serviceOffersOffset;
}

KServiceType::~KServiceType()
{
}

QString KServiceType::parentServiceType() const
{
    const QVariant v = property(QStringLiteral("X-KDE-Derived"));
    return v.toString();
}

bool KServiceType::inherits(const QString &servTypeName) const
{
    if (name() == servTypeName) {
        return true;
    }
    QString st = parentServiceType();
    while (!st.isEmpty()) {
        KServiceType::Ptr ptr = KServiceType::serviceType(st);
        if (!ptr) {
            return false; // error
        }
        if (ptr->name() == servTypeName) {
            return true;
        }
        st = ptr->parentServiceType();
    }
    return false;
}

QVariant KServiceTypePrivate::property(const QString &_name) const
{
    QVariant v;

    if (_name == QLatin1String("Name")) {
        v = QVariant(m_strName);
    } else if (_name == QLatin1String("Comment")) {
        v = QVariant(m_strComment);
    } else {
        v = m_mapProps.value(_name);
    }

    return v;
}

QStringList KServiceTypePrivate::propertyNames() const
{
    QStringList res = m_mapProps.keys();
    res.append(QStringLiteral("Name"));
    res.append(QStringLiteral("Comment"));
    return res;
}

QVariant::Type KServiceType::propertyDef(const QString &_name) const
{
    Q_D(const KServiceType);
    return d->m_mapPropDefs.value(_name, QVariant::Invalid);
}

QStringList KServiceType::propertyDefNames() const
{
    Q_D(const KServiceType);
    return d->m_mapPropDefs.keys();
}

KServiceType::Ptr KServiceType::serviceType(const QString &_name)
{
    KSycoca::self()->ensureCacheValid();
    return KSycocaPrivate::self()->serviceTypeFactory()->findServiceTypeByName(_name);
}

KServiceType::List KServiceType::allServiceTypes()
{
    KSycoca::self()->ensureCacheValid();
    return KSycocaPrivate::self()->serviceTypeFactory()->allServiceTypes();
}

KServiceType::Ptr KServiceType::parentType()
{
    Q_D(KServiceType);
    if (d->m_parentTypeLoaded) {
        return d->parentType;
    }

    d->m_parentTypeLoaded = true;

    const QString parentSt = parentServiceType();
    if (parentSt.isEmpty()) {
        return KServiceType::Ptr();
    }

    KSycoca::self()->ensureCacheValid();
    d->parentType = KSycocaPrivate::self()->serviceTypeFactory()->findServiceTypeByName(parentSt);
    if (!d->parentType) {
        qCWarning(SERVICES) << entryPath() << "specifies undefined MIME type/servicetype" << parentSt;
    }
    return d->parentType;
}

void KServiceType::setServiceOffersOffset(int offset)
{
    Q_D(KServiceType);
    Q_ASSERT(offset != -1);
    d->m_serviceOffersOffset = offset;
}

int KServiceType::serviceOffersOffset() const
{
    Q_D(const KServiceType);
    return d->serviceOffersOffset();
}

QString KServiceType::comment() const
{
    Q_D(const KServiceType);
    return d->comment();
}

bool KServiceType::isDerived() const
{
    Q_D(const KServiceType);
    return d->m_bDerived;
}

QMap<QString, QVariant::Type> KServiceType::propertyDefs() const
{
    Q_D(const KServiceType);
    return d->m_mapPropDefs;
}
