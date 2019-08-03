/*  This file is part of the KDE libraries
 *  Copyright (C) 1999-2000 Waldo Bastian <bastian@kde.org>
 *  Copyright (C) 2005-2009 David Faure <faure@kde.org>
 *  Copyright (C) 2008 Hamish Rodda <rodda@kde.org>
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

#ifndef KSYCOCA_P_H
#define KSYCOCA_P_H

#include "ksycocafactory_p.h"
#include <QStringList>
#include <QElapsedTimer>
#include <QDateTime>
#include <kdirwatch.h>
class QFile;
class QDataStream;
class KSycocaAbstractDevice;
class KMimeTypeFactory;
class KServiceTypeFactory;
class KServiceFactory;
class KServiceGroupFactory;

// This is for the part of the global header that we don't need to store,
// i.e. it's just a struct for returning temp data from readSycocaHeader().
struct KSycocaHeader {
    KSycocaHeader() : timeStamp(0), updateSignature(0) {}
    QString prefixes;
    QString language;
    qint64 timeStamp; // in ms
    quint32 updateSignature;
};

QDataStream &operator>>(QDataStream &in, KSycocaHeader &h);

/**
 * \internal
 * Exported for unittests
 */
class KSERVICE_EXPORT KSycocaPrivate
{
public:
    explicit KSycocaPrivate(KSycoca *q);

    // per-thread "singleton"
    static KSycocaPrivate *self() { return KSycoca::self()->d; }

    bool checkVersion();
    bool openDatabase();
    enum BehaviorIfNotFound {
        IfNotFoundDoNothing = 0,
        IfNotFoundRecreate = 1
    };
    Q_DECLARE_FLAGS(BehaviorsIfNotFound, BehaviorIfNotFound)
    bool checkDatabase(BehaviorsIfNotFound ifNotFound);
    void closeDatabase();
    void setStrategyFromString(const QString &strategy);
    bool tryMmap();

    /**
     * Check if the on-disk cache needs to be rebuilt, and do it then.
     */
    void checkDirectories();

    /**
     * Check if the on-disk cache needs to be rebuilt, and return true
     */
    bool needsRebuild();

    /**
     * Recreate the cache and reopen the database
     */
    bool buildSycoca();

    KSycocaHeader readSycocaHeader();

    KSycocaAbstractDevice *device();
    QDataStream *&stream();

    QString findDatabase();
    void slotDatabaseChanged();

    KMimeTypeFactory *mimeTypeFactory();
    KServiceTypeFactory *serviceTypeFactory();
    KServiceFactory *serviceFactory();
    KServiceGroupFactory *serviceGroupFactory();

    void addLocalResourceDir(const QString &path);

    enum {
        DatabaseNotOpen, // openDatabase must be called
        BadVersion, // it's opened, but it's not useable
        DatabaseOK
    } databaseStatus;
    bool readError;

    qint64 timeStamp; // in ms since epoch
    enum { StrategyMmap, StrategyMemFile, StrategyFile } m_sycocaStrategy;
    QString m_databasePath;
    QStringList changeList;
    QString language;
    quint32 updateSig;
    QMap<QString, qint64> allResourceDirs; // path, modification time in "ms since epoch"

    void addFactory(KSycocaFactory *factory)
    {
        m_factories.append(factory);
    }
    KSycocaFactoryList *factories()
    {
        return &m_factories;
    }

    QElapsedTimer m_lastCheck;
    QDateTime m_dbLastModified;

    // Using KDirWatch because it will reliably tell us every time ksycoca is recreated.
    // QFileSystemWatcher's inotify implementation easily gets confused between "removed" and "changed",
    // and fails to re-add an inotify watch after the file was replaced at some point (KServiceTest::testThreads),
    // thinking it only got changed and not removed+recreated.
    KDirWatch m_fileWatcher;
    bool m_haveListeners;

    KSycoca *q;
private:
    KSycocaFactoryList m_factories;
    size_t sycoca_size;
    const char *sycoca_mmap;
    QFile *m_mmapFile;
    KSycocaAbstractDevice *m_device;

public:
    KMimeTypeFactory *m_mimeTypeFactory;
    KServiceTypeFactory *m_serviceTypeFactory;
    KServiceFactory *m_serviceFactory;
    KServiceGroupFactory *m_serviceGroupFactory;
};

#endif /* KSYCOCA_P_H */

