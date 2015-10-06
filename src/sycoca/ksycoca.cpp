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

#include "ksycoca.h"
#include "ksycoca_p.h"
#include "ksycocautils_p.h"
#include "ksycocatype.h"
#include "ksycocafactory_p.h"
#include "kconfiggroup.h"
#include "ksharedconfig.h"
#include "sycocadebug.h"

#include <qstandardpaths.h>
#include <QtCore/QDataStream>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QThread>
#include <QThreadStorage>
#include <QMetaMethod>

#include <stdlib.h>
#include <fcntl.h>
#include <QDir>
#include <kmimetypefactory_p.h>
#include <kservicetypefactory_p.h>
#include <kservicegroupfactory_p.h>
#include <kservicefactory_p.h>
#include <QCryptographicHash>

#include "kbuildsycoca_p.h"
#include "ksycocadevices_p.h"

#ifdef Q_OS_UNIX
#include <utime.h>
#include <sys/time.h>
#endif

/**
 * Sycoca file version number.
 * If the existing file is outdated, it will not get read
 * but instead we'll regenerate a new one.
 * However running apps should still be able to read it, so
 * only add to the data, never remove/modify.
 */
#define KSYCOCA_VERSION 303

#if HAVE_MADVISE || HAVE_MMAP
#include <sys/mman.h> // This #include was checked when looking for posix_madvise
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif

// The following limitations are in place:
// Maximum length of a single string: 8192 bytes
// Maximum length of a string list: 1024 strings
// Maximum number of entries: 8192
//
// The purpose of these limitations is to limit the impact
// of database corruption.

Q_DECLARE_OPERATORS_FOR_FLAGS(KSycocaPrivate::BehaviorsIfNotFound)

KSycocaPrivate::KSycocaPrivate(KSycoca *q)
    : databaseStatus(DatabaseNotOpen),
      readError(false),
      timeStamp(0),
      m_databasePath(),
      updateSig(0),
      m_haveListeners(false),
      m_globalDatabase(false),
      q(q),
      sycoca_size(0),
      sycoca_mmap(0),
      m_mmapFile(0),
      m_device(0),
      m_mimeTypeFactory(0),
      m_serviceTypeFactory(0),
      m_serviceFactory(0),
      m_serviceGroupFactory(0)
{
#ifdef Q_OS_WIN
    /*
      on windows we use KMemFile (QSharedMemory) to avoid problems
      with mmap (can't delete a mmap'd file)
    */
    m_sycocaStrategy = StrategyMemFile;
#else
    m_sycocaStrategy = StrategyMmap;
#endif
    KConfigGroup config(KSharedConfig::openConfig(), "KSycoca");
    setStrategyFromString(config.readEntry("strategy"));
}

void KSycocaPrivate::setStrategyFromString(const QString &strategy)
{
    if (strategy == QLatin1String("mmap")) {
        m_sycocaStrategy = StrategyMmap;
    } else if (strategy == QLatin1String("file")) {
        m_sycocaStrategy = StrategyFile;
    } else if (strategy == QLatin1String("sharedmem")) {
        m_sycocaStrategy = StrategyMemFile;
    } else if (!strategy.isEmpty()) {
        qCWarning(SYCOCA) << "Unknown sycoca strategy:" << strategy;
    }
}

bool KSycocaPrivate::tryMmap()
{
#if HAVE_MMAP
    Q_ASSERT(!m_databasePath.isEmpty());
    m_mmapFile = new QFile(m_databasePath);
    const bool canRead = m_mmapFile->open(QIODevice::ReadOnly);
    Q_ASSERT(canRead);
    if (!canRead) {
        return false;
    }
    fcntl(m_mmapFile->handle(), F_SETFD, FD_CLOEXEC);
    sycoca_size = m_mmapFile->size();
    void *mmapRet = mmap(0, sycoca_size,
                       PROT_READ, MAP_SHARED,
                       m_mmapFile->handle(), 0);
    /* POSIX mandates only MAP_FAILED, but we are paranoid so check for
       null pointer too.  */
    if (mmapRet == MAP_FAILED || mmapRet == 0) {
        qCDebug(SYCOCA).nospace() << "mmap failed. (length = " << sycoca_size << ")";
        sycoca_mmap = 0;
        return false;
    } else {
        sycoca_mmap = static_cast<const char *>(mmapRet);
#if HAVE_MADVISE
        (void) posix_madvise(mmapRet, sycoca_size, POSIX_MADV_WILLNEED);
#endif // HAVE_MADVISE
        return true;
    }
#else
    return false;
#endif // HAVE_MMAP
}

int KSycoca::version()
{
    return KSYCOCA_VERSION;
}

class KSycocaSingleton
{
public:
    KSycocaSingleton() { }
    ~KSycocaSingleton() { }

    bool hasSycoca() const
    {
        return m_threadSycocas.hasLocalData();
    }
    KSycoca *sycoca()
    {
        if (!m_threadSycocas.hasLocalData()) {
            m_threadSycocas.setLocalData(new KSycoca);
        }
        return m_threadSycocas.localData();
    }
    void setSycoca(KSycoca *s)
    {
        m_threadSycocas.setLocalData(s);
    }

private:
    QThreadStorage<KSycoca *> m_threadSycocas;
};

Q_GLOBAL_STATIC(KSycocaSingleton, ksycocaInstance)

QString KSycocaPrivate::findDatabase()
{
    Q_ASSERT(databaseStatus == DatabaseNotOpen);

    m_globalDatabase = false;
    QString path = KSycoca::absoluteFilePath();
    QFileInfo info(path);
    bool canRead = info.isReadable();
    if (!canRead) {
        path = KSycoca::absoluteFilePath(KSycoca::GlobalDatabase);
        if (!path.isEmpty()) {
            info.setFile(path);
            canRead = info.isReadable();
            if (canRead) {
                m_globalDatabase = true;
            }
        }
    }
    if (canRead) {
        if (m_haveListeners) {
            m_fileWatcher.addFile(path);
        }
        return path;
    }
    return QString();
}

// Read-only constructor
// One instance per thread
KSycoca::KSycoca()
    : d(new KSycocaPrivate(this))
{
    connect(&d->m_fileWatcher, &KDirWatch::created, this, [this](){ d->slotDatabaseChanged(); });
}

bool KSycocaPrivate::openDatabase(bool openDummyIfNotFound)
{
    Q_ASSERT(databaseStatus == DatabaseNotOpen);

    delete m_device; m_device = 0;

    if (m_databasePath.isEmpty()) {
        m_databasePath = findDatabase();
    }

    bool result = true;
    if (!m_databasePath.isEmpty()) {
        qCDebug(SYCOCA) << "Opening ksycoca from" << m_databasePath;
        m_dbLastModified = QFileInfo(m_databasePath).lastModified();
        checkVersion();
    } else { // No database file
        //qCDebug(SYCOCA) << "Could not open ksycoca";
        m_databasePath.clear();
        if (openDummyIfNotFound) {
            // We open a dummy database instead.
            //qCDebug(SYCOCA) << "No database, opening a dummy one.";

            m_sycocaStrategy = StrategyDummyBuffer;
            QDataStream *str = stream();
            *str << qint32(KSYCOCA_VERSION);
            *str << qint32(0);
        } else {
            result = false;
        }
    }
    return result;
}

KSycocaAbstractDevice *KSycocaPrivate::device()
{
    if (m_device) {
        return m_device;
    }

    Q_ASSERT(!m_databasePath.isEmpty());

    KSycocaAbstractDevice *device = m_device;
    if (m_sycocaStrategy == StrategyDummyBuffer) {
        device = new KSycocaBufferDevice;
        device->device()->open(QIODevice::ReadOnly); // can't fail
    } else {
#if HAVE_MMAP
        if (m_sycocaStrategy == StrategyMmap && tryMmap()) {
            device = new KSycocaMmapDevice(sycoca_mmap,
                                           sycoca_size);
            if (!device->device()->open(QIODevice::ReadOnly)) {
                delete device; device = 0;
            }
        }
#endif
#ifndef QT_NO_SHAREDMEMORY
        if (!device && m_sycocaStrategy == StrategyMemFile) {
            device = new KSycocaMemFileDevice(m_databasePath);
            if (!device->device()->open(QIODevice::ReadOnly)) {
                delete device; device = 0;
            }
        }
#endif
        if (!device) {
            device = new KSycocaFileDevice(m_databasePath);
            if (!device->device()->open(QIODevice::ReadOnly)) {
                qCWarning(SYCOCA) << "Couldn't open" << m_databasePath << "even though it is readable? Impossible.";
                //delete device; device = 0; // this would crash in the return statement...
            }
        }
    }
    if (device) {
        m_device = device;
    }
    return m_device;
}

QDataStream *&KSycocaPrivate::stream()
{
    if (!m_device) {
        if (databaseStatus == DatabaseNotOpen) {
            checkDatabase(KSycocaPrivate::IfNotFoundRecreate | KSycocaPrivate::IfNotFoundOpenDummy);
        }

        device(); // create m_device
    }

    return m_device->stream();
}

void KSycocaPrivate::slotDatabaseChanged()
{
    // We don't have information anymore on what resources changed, so emit them all
    changeList = QStringList() << "services" << "servicetypes" << "xdgdata-mime" << "apps";

    qCDebug(SYCOCA) << QThread::currentThread() << "got a notifyDatabaseChanged signal";
    // kbuildsycoca tells us the database file changed
    // We would have found out in the next call to ensureCacheValid(), but for
    // now keep the call to closeDatabase, to help refcounting to 0 the old mmaped file earlier.
    closeDatabase();
    // Start monitoring the new file right away
    m_databasePath = findDatabase();

    // Now notify applications
    emit q->databaseChanged();
    emit q->databaseChanged(changeList);
}

KMimeTypeFactory *KSycocaPrivate::mimeTypeFactory()
{
    if (!m_mimeTypeFactory) {
        m_mimeTypeFactory = new KMimeTypeFactory(q);
    }
    return m_mimeTypeFactory;
}

KServiceTypeFactory *KSycocaPrivate::serviceTypeFactory()
{
    if (!m_serviceTypeFactory) {
        m_serviceTypeFactory = new KServiceTypeFactory(q);
    }
    return m_serviceTypeFactory;
}

KServiceFactory *KSycocaPrivate::serviceFactory()
{
    if (!m_serviceFactory) {
        m_serviceFactory = new KServiceFactory(q);
    }
    return m_serviceFactory;
}

KServiceGroupFactory *KSycocaPrivate::serviceGroupFactory()
{
    if (!m_serviceGroupFactory) {
        m_serviceGroupFactory = new KServiceGroupFactory(q);
    }
    return m_serviceGroupFactory;
}

// Add local paths to the list of dirs we got from the global database
void KSycocaPrivate::addLocalResourceDir(const QString &path)
{
    // If any local path is more recent than the time the global sycoca was created, build a local sycoca.
    allResourceDirs.insert(path, timeStamp);
}

// Read-write constructor - only for KBuildSycoca
KSycoca::KSycoca(bool /* dummy */)
    : d(new KSycocaPrivate(this))
{
}

KSycoca *KSycoca::self()
{
    KSycoca *s = ksycocaInstance()->sycoca();
    Q_ASSERT(s);
    return s;
}

KSycoca::~KSycoca()
{
    d->closeDatabase();
    delete d;
    //if (ksycocaInstance.exists()
    //    && ksycocaInstance->self == this)
    //    ksycocaInstance->self = 0;
}

bool KSycoca::isAvailable() // TODO KF6: make it non-static (mostly useful for unittests)
{
    return self()->d->checkDatabase(KSycocaPrivate::IfNotFoundDoNothing/* don't open dummy db if not found */);
}

void KSycocaPrivate::closeDatabase()
{
    delete m_device;
    m_device = 0;

    // It is very important to delete all factories here
    // since they cache information about the database file
    // But other threads might be using them, so this class is
    // refcounted, and deleted when the last thread is done with them
    qDeleteAll(m_factories);
    m_factories.clear();

    m_mimeTypeFactory = 0;
    m_serviceFactory = 0;
    m_serviceTypeFactory = 0;
    m_serviceGroupFactory = 0;

#if HAVE_MMAP
    if (sycoca_mmap) {
        // Solaris has munmap(char*, size_t) and everything else should
        // be happy with a char* for munmap(void*, size_t)
        munmap(const_cast<char *>(sycoca_mmap), sycoca_size);
        sycoca_mmap = 0;
    }
    delete m_mmapFile; m_mmapFile = 0;
#endif

    databaseStatus = DatabaseNotOpen;
    m_databasePath.clear();
    timeStamp = 0;
}

void KSycoca::addFactory(KSycocaFactory *factory)
{
    d->addFactory(factory);
}

#ifndef KSERVICE_NO_DEPRECATED
bool KSycoca::isChanged(const char *type)
{
    return self()->d->changeList.contains(QString::fromLatin1(type));
}
#endif

QDataStream *KSycoca::findEntry(int offset, KSycocaType &type)
{
    QDataStream *str = stream();
    Q_ASSERT(str);
    //qCDebug(SYCOCA) << QString("KSycoca::_findEntry(offset=%1)").arg(offset,8,16);
    str->device()->seek(offset);
    qint32 aType;
    *str >> aType;
    type = KSycocaType(aType);
    //qCDebug(SYCOCA) << QString("KSycoca::found type %1").arg(aType);
    return str;
}

KSycocaFactoryList *KSycoca::factories()
{
    return d->factories();
}

// Warning, checkVersion rewinds to the beginning of stream().
bool KSycocaPrivate::checkVersion()
{
    QDataStream *m_str = device()->stream();
    Q_ASSERT(m_str);
    m_str->device()->seek(0);
    qint32 aVersion;
    *m_str >> aVersion;
    if (aVersion < KSYCOCA_VERSION) {
        qCDebug(SYCOCA) << "Found version" << aVersion << ", expecting version" << KSYCOCA_VERSION << "or higher.";
        databaseStatus = BadVersion;
        return false;
    } else {
        databaseStatus = DatabaseOK;
        return true;
    }
}

// This is now completely useless. KF6: remove
extern KSERVICE_EXPORT bool kservice_require_kded;
KSERVICE_EXPORT bool kservice_require_kded = true;

// If it returns true, we have a valid database and the stream has rewinded to the beginning
// and past the version number.
bool KSycocaPrivate::checkDatabase(BehaviorsIfNotFound ifNotFound)
{
    if (databaseStatus == DatabaseOK) {
        if (checkVersion()) { // we know the version is ok, but we must rewind the stream anyway
            return true;
        }
    }

    closeDatabase(); // close the dummy one

    // Check if new database already available
    if (openDatabase(ifNotFound & IfNotFoundOpenDummy)) {
        if (checkVersion()) {
            // Database exists, and version is ok, we can read it.

            if (qAppName() != KBUILDSYCOCA_EXENAME && ifNotFound != IfNotFoundDoNothing) {

                // Ensure it's uptodate, rebuild if needed
                checkDirectories();

                // Don't check again for some time
                m_lastCheck.start();
            }

            return true;
        }
    }

    if (ifNotFound & IfNotFoundRecreate) {
        return buildSycoca();
    }

    return false;
}

QDataStream *KSycoca::findFactory(KSycocaFactoryId id)
{
    // Ensure we have a valid database (right version, and rewinded to beginning)
    if (!d->checkDatabase(KSycocaPrivate::IfNotFoundRecreate)) {
        return 0;
    }

    QDataStream *str = stream();
    Q_ASSERT(str);

    qint32 aId;
    qint32 aOffset;
    while (true) {
        *str >> aId;
        if (aId == 0) {
            qCWarning(SYCOCA) << "Error, KSycocaFactory (id =" << int(id) << ") not found!";
            break;
        }
        *str >> aOffset;
        if (aId == id) {
            //qCDebug(SYCOCA) << "KSycoca::findFactory(" << id << ") offset " << aOffset;
            str->device()->seek(aOffset);
            return str;
        }
    }
    return 0;
}

bool KSycoca::needsRebuild()
{
    return d->needsRebuild();
}

KSycocaHeader KSycocaPrivate::readSycocaHeader()
{
    KSycocaHeader header;
    // do not try to launch kbuildsycoca from here; this code is also called by kbuildsycoca.
    if (!checkDatabase(KSycocaPrivate::IfNotFoundDoNothing)) {
        return header;
    }
    QDataStream *str = stream();
    qint64 oldPos = str->device()->pos();

    Q_ASSERT(str);
    qint32 aId;
    qint32 aOffset;
    // skip factories offsets
    while (true) {
        *str >> aId;
        if (aId) {
            *str >> aOffset;
        } else {
            break;    // just read 0
        }
    }
    // We now point to the header
    KSycocaUtilsPrivate::read(*str, header.prefixes);
    *str >> header.timeStamp;
    KSycocaUtilsPrivate::read(*str, header.language);
    *str >> header.updateSignature;
    QStringList directoryList;
    KSycocaUtilsPrivate::read(*str, directoryList);
    allResourceDirs.clear();
    for (int i = 0; i < directoryList.count(); ++i) {
        qint64 mtime;
        *str >> mtime;
        allResourceDirs.insert(directoryList.at(i), mtime);
    }

    str->device()->seek(oldPos);

    timeStamp = header.timeStamp;

    // for the useless public accessors. KF6: remove these two lines, the accessors and the vars.
    language = header.language;
    updateSig = header.updateSignature;

    if (m_globalDatabase) {
        // The global database doesn't point to the user's local dirs, but we need to check them too
        // to react on something being created there
        const QString dataHome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        addLocalResourceDir(dataHome + QLatin1String("/kservices5"));
        addLocalResourceDir(dataHome + QLatin1String("/kservicetypes5"));
        addLocalResourceDir(dataHome + QLatin1String("/mime"));
        addLocalResourceDir(dataHome + QLatin1String("/applications"));
    }

    return header;
}


class TimestampChecker
{
public:
    TimestampChecker()
        : m_now(QDateTime::currentDateTime())
    {}

    // Check times of last modification of all directories on which ksycoca depends,
    // If none of them is newer than the mtime we stored for that directory at the
    // last rebuild, this means that there's no need to rebuild ksycoca.
    bool checkTimestamps(const QMap<QString, qint64> &dirs)
    {
        Q_ASSERT(!dirs.isEmpty());
        qCDebug(SYCOCA) << "checking file timestamps";
        for (auto it = dirs.begin(); it != dirs.end(); ++it) {
            const QString dir = it.key();
            const qint64 lastStamp = it.value();

            auto visitor = [&] (const QFileInfo &fi) {
                const QDateTime mtime = fi.lastModified();
                if (mtime.toMSecsSinceEpoch() > lastStamp) {
                    if (mtime > m_now) {
                        qCDebug(SYCOCA) << fi.filePath() << "has a modification time in the future" << mtime;
                    }
                    qCDebug(SYCOCA) << "timestamp changed:" << fi.filePath() << mtime << ">" << lastStamp;
                    // no need to continue search
                    return false;
                }

                return true;
            };

            if (!KSycocaUtilsPrivate::visitResourceDirectory(dir, visitor)) {
                return false;
            }
        }
        return true;
    }

private:
    QDateTime m_now;
};

void KSycocaPrivate::checkDirectories()
{
    if (needsRebuild()) {
        buildSycoca();
    }
}

bool KSycocaPrivate::needsRebuild()
{
    if (!timeStamp && databaseStatus == DatabaseOK) {
        (void) readSycocaHeader();
    }
    // these days timeStamp is really a "bool headerFound", the value itself doesn't matter...
    // KF6: replace it with bool.
    return timeStamp != 0 && !TimestampChecker().checkTimestamps(allResourceDirs);
}

bool KSycocaPrivate::buildSycoca()
{
    KBuildSycoca builder;
    if (!builder.recreate()) {
        return false; // error
    }

    closeDatabase(); // close the dummy one

    // Ok, the new database should be here now, open it.
    if (!openDatabase()) {
        qCDebug(SYCOCA) << "Still no database...";
        return false; // Still no database - uh oh
    }
    if (!checkVersion()) {
        qCDebug(SYCOCA) << "Still outdated...";
        return false; // Still outdated - uh oh
    }
    return true;
}

quint32 KSycoca::timeStamp()
{
    if (!d->timeStamp) {
        (void) d->readSycocaHeader();
    }
    return d->timeStamp / 1000; // from ms to s
}

quint32 KSycoca::updateSignature()
{
    if (!d->timeStamp) {
        (void) d->readSycocaHeader();
    }
    return d->updateSig;
}

QString KSycoca::absoluteFilePath(DatabaseType type)
{
    const QStringList paths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QString suffix = QLatin1Char('_') + QLocale().bcp47Name();

    if (type == GlobalDatabase) {
        const QString fileName = QStringLiteral("ksycoca5") + suffix;
        QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("ksycoca/") + fileName);
        if (path.isEmpty()) {
            // No existing global DB found, maybe we are going to make a new one.
            // We don't want it in $HOME, so skip the first entry in <paths> and pick the second one.
            if (paths.count() == 1) { // unlikely
                return QString();
            }
            return paths.at(1) + QStringLiteral("/ksycoca/") + fileName;
        }
        return path;
    }

    const QByteArray ksycoca_env = qgetenv("KDESYCOCA");
    if (ksycoca_env.isEmpty()) {
        const QByteArray pathHash = QCryptographicHash::hash(paths.join(QString(QLatin1Char(':'))).toUtf8(), QCryptographicHash::Sha1);
        suffix += QLatin1Char('_') + QString::fromLatin1(pathHash.toBase64());
        suffix.replace('/', '_');
#ifdef Q_OS_WIN
        suffix.replace(':', '_');
#endif
        const QString fileName = QStringLiteral("ksycoca5") + suffix;
        return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1Char('/') + fileName;
    } else {
        return QFile::decodeName(ksycoca_env);
    }
}

QString KSycoca::language()
{
    if (d->language.isEmpty()) {
        (void) d->readSycocaHeader();
    }
    return d->language;
}

QStringList KSycoca::allResourceDirs()
{
    if (!d->timeStamp) {
        (void) d->readSycocaHeader();
    }
    return d->allResourceDirs.keys();
}

void KSycoca::flagError()
{
    qCWarning(SYCOCA) << "ERROR: KSycoca database corruption!";
    KSycoca *sycoca = self();
    if (sycoca->d->readError) {
        return;
    }
    sycoca->d->readError = true;
    if (qAppName() != KBUILDSYCOCA_EXENAME && !sycoca->isBuilding()) {
        // Rebuild the damned thing.
        KBuildSycoca builder;
        (void)builder.recreate();
    }
}

bool KSycoca::isBuilding()
{
    return false;
}

void KSycoca::disableAutoRebuild()
{
    qCWarning(SYCOCA) << "KSycoca::disableAutoRebuild() is internal, do not call it.";
}

QDataStream *&KSycoca::stream()
{
    return d->stream();
}

void KSycoca::connectNotify(const QMetaMethod &signal)
{
    if (signal.name() == "databaseChanged" && !d->m_haveListeners) {
        d->m_haveListeners = true;
        if (d->m_databasePath.isEmpty()) {
            d->m_databasePath = d->findDatabase();
        } else {
            d->m_fileWatcher.addFile(d->m_databasePath);
        }
    }
}

void KSycoca::clearCaches()
{
    if (ksycocaInstance.exists() && ksycocaInstance()->hasSycoca())
        ksycocaInstance()->sycoca()->d->closeDatabase();
}

extern KSERVICE_EXPORT int ksycoca_ms_between_checks;
KSERVICE_EXPORT int ksycoca_ms_between_checks = 1500;

void KSycoca::ensureCacheValid()
{
    if (qAppName() == KBUILDSYCOCA_EXENAME) {
        return;
    }

    if (d->databaseStatus != KSycocaPrivate::DatabaseOK) {
        if (!d->checkDatabase(KSycocaPrivate::IfNotFoundRecreate)) {
            return;
        }
    }

    if (d->m_lastCheck.isValid() && d->m_lastCheck.elapsed() < ksycoca_ms_between_checks) {
        return;
    }
    d->m_lastCheck.start();

    // Check if the file on disk was modified since we last checked it.
    QFileInfo info(d->m_databasePath);
    if (info.lastModified() == d->m_dbLastModified) {
        // Check if the watched directories were modified, then the cache needs a rebuild.
        d->checkDirectories();
        return;
    }

    // Close the database and forget all about what we knew.
    // The next call to any public method will recreate
    // everything that's needed.
    d->closeDatabase();
}
