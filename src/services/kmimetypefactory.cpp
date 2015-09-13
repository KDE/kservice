/*  This file is part of the KDE libraries
 *  Copyright (C) 1999 Waldo Bastian <bastian@kde.org>
 *  Copyright (C) 2006-2009 David Faure <faure@kde.org>
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
 */

#include "kmimetypefactory_p.h"
#include <ksycoca.h>
#include <ksycocadict_p.h>
#include <QDebug>

extern int servicesDebugArea();

KMimeTypeFactory::KMimeTypeFactory(KSycoca *db)
    : KSycocaFactory(KST_KMimeTypeFactory, db)
{
}

KMimeTypeFactory::~KMimeTypeFactory()
{
}

int KMimeTypeFactory::entryOffset(const QString &mimeTypeName)
{
    if (!sycocaDict()) {
        return -1;    // Error!
    }
    assert(!sycoca()->isBuilding());
    const int offset = sycocaDict()->find_string(mimeTypeName);
    return offset;
}

int KMimeTypeFactory::serviceOffersOffset(const QString &mimeTypeName)
{
    const int offset = entryOffset(mimeTypeName);
    if (!offset) {
        return -1;    // Not found
    }

    MimeTypeEntry::Ptr newMimeType(createEntry(offset));
    if (!newMimeType) {
        return -1;
    }
    // Check whether the dictionary was right.
    if (newMimeType->name() != mimeTypeName) {
        // No it wasn't...
        return -1;
    }
    return newMimeType->serviceOffersOffset();
}

KMimeTypeFactory::MimeTypeEntry *KMimeTypeFactory::createEntry(int offset) const
{
    KSycocaType type;
    QDataStream *str = sycoca()->findEntry(offset, type);
    if (!str) {
        return 0;
    }

    if (type != KST_KMimeTypeEntry) {
        qWarning() << "KMimeTypeFactory: unexpected object entry in KSycoca database (type=" << int(type) << ")";
        return 0;
    }
    MimeTypeEntry *newEntry = new MimeTypeEntry(*str, offset);
    if (newEntry && !newEntry->isValid()) {
        qWarning() << "KMimeTypeFactory: corrupt object in KSycoca database!\n";
        delete newEntry;
        newEntry = 0;
    }
    return newEntry;
}

QStringList KMimeTypeFactory::allMimeTypes()
{
    // TODO: reimplement in terms of "listing xdgdata-mime", to avoid ksycoca dependency,
    // then move to KMimeTypeRepository
    QStringList result;
    const KSycocaEntry::List list = allEntries();
    for (KSycocaEntry::List::ConstIterator it = list.begin();
            it != list.end();
            ++it) {
        Q_ASSERT((*it)->isType(KST_KMimeTypeEntry));
        MimeTypeEntry::Ptr mimeType(static_cast<MimeTypeEntry*>((*it).data()));
        result.append(mimeType->name());
    }
    return result;
}

QStringList KMimeTypeFactory::resourceDirs()
{
    return KSycocaFactory::allDirectories(QStringLiteral("mime"));
}

////

class KMimeTypeFactory::MimeTypeEntryPrivate : public KSycocaEntryPrivate
{
public:
    K_SYCOCATYPE(KST_KMimeTypeEntry, KSycocaEntryPrivate)
    MimeTypeEntryPrivate(const QString &file, const QString &name)
        : KSycocaEntryPrivate(file), m_name(name), m_serviceOffersOffset(-1)
    {
    }
    MimeTypeEntryPrivate(QDataStream &s, int offset)
        : KSycocaEntryPrivate(s, offset), m_serviceOffersOffset(-1)
    {
        s >> m_name >> m_serviceOffersOffset;
    }
    virtual QString name() const
    {
        return m_name;
    }
    virtual void save(QDataStream &s);

    QString m_name;
    int m_serviceOffersOffset;
};

void KMimeTypeFactory::MimeTypeEntryPrivate::save(QDataStream &s)
{
    KSycocaEntryPrivate::save(s);
    s << m_name << m_serviceOffersOffset;
}

////

KMimeTypeFactory::MimeTypeEntry::MimeTypeEntry(const QString &file, const QString &name)
    : KSycocaEntry(*new MimeTypeEntryPrivate(file, name))
{
}

KMimeTypeFactory::MimeTypeEntry::MimeTypeEntry(QDataStream &s, int offset)
    : KSycocaEntry(*new MimeTypeEntryPrivate(s, offset))
{
}

KMimeTypeFactory::MimeTypeEntry::~MimeTypeEntry() {}

int KMimeTypeFactory::MimeTypeEntry::serviceOffersOffset() const
{
    Q_D(const MimeTypeEntry);
    return d->m_serviceOffersOffset;
}

void KMimeTypeFactory::MimeTypeEntry::setServiceOffersOffset(int off)
{
    Q_D(MimeTypeEntry);
    d->m_serviceOffersOffset = off;
}

