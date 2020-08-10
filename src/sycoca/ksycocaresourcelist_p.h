/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KSYCOCARESOURCELIST_H
#define KSYCOCARESOURCELIST_H

#include <QLinkedList>
#include <QString>

struct KSycocaResource {
    QByteArray resource;
    QString subdir;
    QString extension;
};

class KSycocaResourceList : public QLinkedList<KSycocaResource>
{
public:
    KSycocaResourceList() { }

    // resource is just used in the databaseChanged signal
    // subdir is always under QStandardPaths::GenericDataLocation. E.g. mime, kservices5, etc.
    void add(const QByteArray &resource, const QString &subdir, const QString &filter)
    {
        KSycocaResource res;
        res.resource = resource;
        res.subdir = subdir;
        res.extension = filter.mid(1);
        append(res);
    }
};

#endif
