/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kserviceaction.h"
#include "kservice.h"
#include <QVariant>
#include <QDataStream>

class KServiceActionPrivate : public QSharedData
{
public:
    KServiceActionPrivate(const QString &name, const QString &text,
                          const QString &icon, const QString &exec,
                          bool noDisplay)
        : m_name(name), m_text(text), m_icon(icon), m_exec(exec), m_noDisplay(noDisplay) {}
    QString m_name;
    QString m_text;
    QString m_icon;
    QString m_exec;
    QVariant m_data;
    bool m_noDisplay;
    KServicePtr m_service;
    // warning keep QDataStream operators in sync if adding data here
};

KServiceAction::KServiceAction()
    : d(new KServiceActionPrivate(QString(), QString(), QString(), QString(), false))
{
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 69)
KServiceAction::KServiceAction(const QString &name, const QString &text,
                               const QString &icon, const QString &exec,
                               bool noDisplay)
    : d(new KServiceActionPrivate(name, text, icon, exec, noDisplay))
{
}
#endif

KServiceAction::KServiceAction(const QString &name, const QString &text,
                               const QString &icon, const QString &exec,
                               bool noDisplay, const KServicePtr &service)
    : d(new KServiceActionPrivate(name, text, icon, exec, noDisplay))
{
    d->m_service = service;
}

KServiceAction::~KServiceAction()
{
}

KServiceAction::KServiceAction(const KServiceAction &other)
    : d(other.d)
{
}

KServiceAction &KServiceAction::operator=(const KServiceAction &other)
{
    d = other.d;
    return *this;
}

QVariant KServiceAction::data() const
{
    return d->m_data;
}

void KServiceAction::setData(const QVariant &data)
{
    d->m_data = data;
}

QString KServiceAction::name() const
{
    return d->m_name;
}

QString KServiceAction::text() const
{
    return d->m_text;
}

QString KServiceAction::icon() const
{
    return d->m_icon;
}

QString KServiceAction::exec() const
{
    return d->m_exec;
}

bool KServiceAction::noDisplay() const
{
    return d->m_noDisplay;
}

bool KServiceAction::isSeparator() const
{
    return d->m_name == QLatin1String("_SEPARATOR_");
}

KServicePtr KServiceAction::service() const
{
    return d->m_service;
}

void KServiceAction::setService(const KServicePtr &service)
{
    d->m_service = service;
}

QDataStream &operator>>(QDataStream &str, KServiceAction &act)
{
    KServiceActionPrivate *d = act.d;
    str >> d->m_name;
    str >> d->m_text;
    str >> d->m_icon;
    str >> d->m_exec;
    str >> d->m_data;
    str >> d->m_noDisplay;
    return str;
}

QDataStream &operator<<(QDataStream &str, const KServiceAction &act)
{
    const KServiceActionPrivate *d = act.d;
    str << d->m_name;
    str << d->m_text;
    str << d->m_icon;
    str << d->m_exec;
    str << d->m_data;
    str << d->m_noDisplay;
    return str;
}
