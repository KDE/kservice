/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 1999-2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSERVICEPRIVATE_H
#define KSERVICEPRIVATE_H

#include "kservice.h"
#include <QVector>

#include <ksycocaentry_p.h>

class KServicePrivate : public KSycocaEntryPrivate
{
public:
    K_SYCOCATYPE(KST_KService, KSycocaEntryPrivate)

    explicit KServicePrivate(const QString &path)
        : KSycocaEntryPrivate(path)
        , m_bValid(true)
    {
    }
    KServicePrivate(QDataStream &_str, int _offset)
        : KSycocaEntryPrivate(_str, _offset)
        , m_bValid(true)
    {
        load(_str);
    }
    KServicePrivate(const KServicePrivate &other) = default;

    void init(const KDesktopFile *config, KService *q);

    void parseActions(const KDesktopFile *config, KService *q);
    void load(QDataStream &);
    void save(QDataStream &) override;

    QString name() const override
    {
        return m_strName;
    }

    QString storageId() const override
    {
        if (!menuId.isEmpty()) {
            return menuId;
        }
        return path;
    }

    bool isValid() const override
    {
        return m_bValid;
    }

    QVariant property(const QString &name) const override;

    QStringList propertyNames() const override;

    QVariant property(const QString &_name, QVariant::Type t) const;

    QStringList serviceTypes() const;

    QStringList categories;
    QString menuId;
    QString m_strType;
    QString m_strName;
    QString m_strExec;
    QString m_strIcon;
    QString m_strTerminalOptions;
    QString m_strWorkingDirectory;
    QString m_strComment;
    QString m_strLibrary;

    int m_initialPreference; // deprecated
    // the initial preference is per-servicetype now.
    QVector<KService::ServiceTypeAndPreference> m_serviceTypes;

    QString m_strDesktopEntryName;
    KService::DBusStartupType m_DBUSStartusType;
    QMap<QString, QVariant> m_mapProps;
    QStringList m_lstFormFactors;
    QStringList m_lstKeywords;
    QString m_strGenName;
    QList<KServiceAction> m_actions;
    bool m_bAllowAsDefault : 1;
    bool m_bTerminal : 1;
    bool m_bValid : 1;
};
#endif
