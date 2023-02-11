/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kservicetype.h"
#include "kservicetype_p.h"
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
            QVariant variant(QMetaType::fromName(cg.readEntry("Type").toLatin1().constData()));
            variant = cg.readEntry("Value", variant);

            if (variant.isValid()) {
                m_mapProps.insert(groupName.mid(marker.size()), variant);
            }
        } else if (QLatin1String marker("PropertyDef::"); groupName.startsWith(marker)) {
            KConfigGroup cg(config, groupName);
            m_mapPropDefs.insert(groupName.mid(marker.size()),
                                 static_cast<QMetaType::Type>(QMetaType::fromName(cg.readEntry("Type").toLatin1().constData()).id()));
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

QMap<QString, QMetaType::Type> KServiceType::propertyDefs() const
{
    Q_D(const KServiceType);
    return d->m_mapPropDefs;
}
