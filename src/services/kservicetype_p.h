/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __kservicetype_p_h__
#define __kservicetype_p_h__

#include "kservicetype.h"
#include <ksycocaentry_p.h>

class KServiceTypePrivate : public KSycocaEntryPrivate
{
public:
    K_SYCOCATYPE(KST_KServiceType, KSycocaEntryPrivate)

    explicit KServiceTypePrivate(const QString &path)
        : KSycocaEntryPrivate(path)
        , m_serviceOffersOffset(-1)
        , m_bDerived(false)
        , m_parentTypeLoaded(false)
    {
    }

    KServiceTypePrivate(QDataStream &_str, int offset)
        : KSycocaEntryPrivate(_str, offset)
        , m_serviceOffersOffset(-1)
        , m_bDerived(false)
        , m_parentTypeLoaded(false)
    {
    }

    ~KServiceTypePrivate() override
    {
    }

    void save(QDataStream &) override;

    QString name() const override
    {
        return m_strName;
    }

    QVariant property(const QString &name) const override;

    QStringList propertyNames() const override;

    virtual QString comment() const
    {
        return m_strComment;
    }

    virtual int serviceOffersOffset() const
    {
        return m_serviceOffersOffset;
    }

    void init(KDesktopFile *config);
    void load(QDataStream &_str);

    KServiceType::Ptr parentType;
    QString m_strName;
    mutable /*remove mutable when kmimetype doesn't use this anymore*/ QString m_strComment;
    int m_serviceOffersOffset;
    QMap<QString, QVariant::Type> m_mapPropDefs;
    QMap<QString, QVariant> m_mapProps;
    unsigned m_bDerived : 1;
    unsigned m_parentTypeLoaded : 1;
};

#endif // __kservicetype_p_h__
