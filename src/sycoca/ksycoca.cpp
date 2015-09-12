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
#include <QProcess>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>

#include <stdlib.h>
#include <fcntl.h>
#include <QDir>

#include "ksycocadevices_p.h"

/**
 * Sycoca file version number.
 * If the existing file is outdated, it will not get read
 * but instead we'll ask kded to regenerate a new one...
 */
#define KSYCOCA_VERSION 302

/**
 * Sycoca file name, used internally (by kbuildsycoca)
 */
#define KSYCOCA_FILENAME "ksycoca5"

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

KSycocaPrivate::KSycocaPrivate()
    : databaseStatus(DatabaseNotOpen),
      readError(false),
      timeStamp(0),
      m_databasePath(),
      updateSig(0),
      sycoca_size(0),
      sycoca_mmap(0),
      m_mmapFile(0),
      m_device(0)
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
        qWarning() << "Unknown sycoca strategy:" << strategy;
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

// Read-only constructor
KSycoca::KSycoca()
    : d(new KSycocaPrivate)
{
    QDBusConnection::sessionBus().connect(QString(), QString(),
                                          QString::fromLatin1("org.kde.KSycoca"),
                                          QString::fromLatin1("notifyDatabaseChanged"),
                                          this, SLOT(slotNotifyDatabaseChanged(QStringList)));
}

bool KSycocaPrivate::openDatabase(bool openDummyIfNotFound)
{
    Q_ASSERT(databaseStatus == DatabaseNotOpen);

    delete m_device; m_device = 0;
    QString path = KSycoca::absoluteFilePath();

    QFileInfo info(path);
    bool canRead = info.isReadable();
    qCDebug(SYCOCA) << "Trying to open ksycoca from" << path;
    if (!canRead) {
        path = KSycoca::absoluteFilePath(KSycoca::GlobalDatabase);
        if (!path.isEmpty()) {
            qCDebug(SYCOCA) << "Trying to open global ksycoca from " << path;
            info.setFile(path);
            canRead = info.isReadable();
        }
    }

    bool result = true;
    if (canRead) {
        m_databasePath = path;
        m_dbLastModified = info.lastModified();
        checkVersion();
    } else { // No database file
        //qCDebug(SYCOCA) << "Could not open ksycoca";
        m_databasePath.clear();
        databaseStatus = NoDatabase;
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
                qWarning() << "Couldn't open" << m_databasePath << "even though it is readable? Impossible.";
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

// Read-write constructor - only for KBuildSycoca
KSycoca::KSycoca(bool /* dummy */)
    : d(new KSycocaPrivate)
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

bool KSycoca::isAvailable()
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

void KSycoca::slotNotifyDatabaseChanged(const QStringList &changeList)
{
    d->changeList = changeList;
    //qCDebug(SYCOCA) << QThread::currentThread() << "got a notifyDatabaseChanged signal" << changeList;
    // kbuildsycoca tells us the database file changed
    // We would have found out in the next call to ensureCacheValid(), but for
    // now keep the call to closeDatabase, to help refcounting to 0 the old mmaped file earlier.
    d->closeDatabase();

    // Now notify applications
#ifndef KSERVICE_NO_DEPRECATED
    emit databaseChanged();
#endif
    emit databaseChanged(changeList);
}

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

            if (qAppName() != KBUILDSYCOCA_EXENAME) {

                // Ensure it's uptodate, rebuild if needed
                checkDirectories(ifNotFound);

                // Don't check again for some time
                m_lastCheck.start();
            }

            return true;
        }
    }

    if (ifNotFound & IfNotFoundRecreate) {
        return buildSycoca(ifNotFound);
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
            qWarning() << "Error, KSycocaFactory (id =" << int(id) << ") not found!";
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

KSycoca::KSycocaHeader KSycoca::readSycocaHeader()
{
    return d->readSycocaHeader();
}

bool KSycoca::needsRebuild()
{
    return d->needsRebuild();
}

KSycoca::KSycocaHeader KSycocaPrivate::readSycocaHeader()
{
    KSycoca::KSycocaHeader header;
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
    KSycocaUtilsPrivate::read(*str, allResourceDirs);

    str->device()->seek(oldPos);

    timeStamp = header.timeStamp;

    // for the useless public accessors. KF6: remove these two lines, the accessors and the vars.
    language = header.language;
    updateSig = header.updateSignature;

    return header;
}

static bool checkDirTimestamps(const QString &dirname, const QDateTime &stamp)
{
    QDir dir(dirname);
    const QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::Unsorted);
    if (list.isEmpty()) {
        return true;
    }
    foreach (const QFileInfo &fi, list) {
        if (fi.lastModified() > stamp) {
            qCDebug(SYCOCA) << "timestamp changed:" << fi.filePath() << fi.lastModified() << ">" << stamp;
            return false;
        }
        if (fi.isDir() && !checkDirTimestamps(fi.filePath(), stamp)) {
            return false;
        }
    }
    return true;
}

// Check times of last modification of all directories on which ksycoca depends,
// If all of them are older than the timestamp in file ksycocastamp, this
// means that there's no need to rebuild ksycoca.
// kbuildsycoca has a similar check (when launched with --checkstamps, which nobody does),
// but it even looks at every file. We can't afford doing that here.
static bool checkTimestamps(qint64 timestamp, const QStringList &dirs)
{
    qCDebug(SYCOCA) << "checking file timestamps";
    const QDateTime stamp = QDateTime::fromMSecsSinceEpoch(timestamp);
    for (QStringList::ConstIterator it = dirs.begin(); it != dirs.end(); ++it) {
        const QString dir = *it;
        QFileInfo inf(dir);
        if (inf.lastModified() > stamp) {
            qCDebug(SYCOCA) << "timestamp changed:" << dir;
            return false;
        }
        // Recurse only for services and menus.
        // Apps and servicetypes don't need recursion, so save the directory listing.
        if (dir.contains("/applications") || dir.contains("/kservicetypes5")) {
            continue;
        }
        if (!checkDirTimestamps(dir, stamp)) {
            return false;
        }
    }
    return true;
}

void KSycocaPrivate::checkDirectories(BehaviorsIfNotFound ifNotFound)
{
    if (needsRebuild()) {
        buildSycoca(ifNotFound);
    }
}

bool KSycocaPrivate::needsRebuild()
{
    if (!timeStamp && databaseStatus == DatabaseOK) {
        (void) readSycocaHeader();
    }
    return timeStamp != 0 && !checkTimestamps(timeStamp, allResourceDirs);
}

bool KSycocaPrivate::buildSycoca(BehaviorsIfNotFound ifNotFound)
{
    QProcess proc;
    const QString kbuildsycoca = QStandardPaths::findExecutable(KBUILDSYCOCA_EXENAME);
    if (!kbuildsycoca.isEmpty()) {
        QStringList args;
        if (QStandardPaths::isTestModeEnabled()) {
            args << QLatin1String("--testmode");
        }
        proc.setProcessChannelMode(QProcess::MergedChannels); // silence kbuildsycoca output
        proc.start(kbuildsycoca, args);
        proc.waitForFinished();
    }

    closeDatabase(); // close the dummy one

    // Ok, the new database should be here now, open it.
    if (!openDatabase(ifNotFound & IfNotFoundOpenDummy)) {
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
        (void) readSycocaHeader();
    }
    return d->timeStamp / 1000; // from ms to s
}

quint32 KSycoca::updateSignature()
{
    if (!d->timeStamp) {
        (void) readSycocaHeader();
    }
    return d->updateSig;
}

QString KSycoca::absoluteFilePath(DatabaseType type)
{
    if (type == GlobalDatabase) {
        QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString::fromLatin1("kservices5/" KSYCOCA_FILENAME));
        if (path.isEmpty()) {
            return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QString::fromLatin1("/kservices5/" KSYCOCA_FILENAME);
        }
        return path;
    }

    const QByteArray ksycoca_env = qgetenv("KDESYCOCA");
    if (ksycoca_env.isEmpty()) {
        return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1Char('/') + QString::fromLatin1(KSYCOCA_FILENAME);
    } else {
        return QFile::decodeName(ksycoca_env);
    }
}

QString KSycoca::language()
{
    if (d->language.isEmpty()) {
        (void) readSycocaHeader();
    }
    return d->language;
}

QStringList KSycoca::allResourceDirs()
{
    if (!d->timeStamp) {
        (void) readSycocaHeader();
    }
    return d->allResourceDirs;
}

void KSycoca::flagError()
{
    qWarning() << "ERROR: KSycoca database corruption!";
    KSycocaPrivate *d = ksycocaInstance()->sycoca()->d;
    if (d->readError) {
        return;
    }
    d->readError = true;
    if (qAppName() != KBUILDSYCOCA_EXENAME) {
        // Rebuild the damned thing.
        if (QProcess::execute(QStandardPaths::findExecutable(QString::fromLatin1(KBUILDSYCOCA_EXENAME))) != 0) {
            qWarning("ERROR: Running %s failed", KBUILDSYCOCA_EXENAME);
        }
        // Old comment, maybe not true anymore:
        // Do not wait until the DBUS signal from kbuildsycoca here.
        // It deletes m_str which is a problem when flagError is called during the KSycocaFactory ctor...
    }
}

bool KSycoca::isBuilding()
{
    return false;
}

void KSycoca::disableAutoRebuild()
{
    qWarning("KSycoca::disableAutoRebuild() is internal, do not call it.");
}

QDataStream *&KSycoca::stream()
{
    return d->stream();
}

void KSycoca::clearCaches()
{
    if (ksycocaInstance.exists() && ksycocaInstance()->hasSycoca())
        ksycocaInstance()->sycoca()->d->closeDatabase();
}

void KSycoca::ensureCacheValid()
{
    if (qAppName() == KBUILDSYCOCA_EXENAME) {
        return;
    }

    if (d->m_databasePath.isEmpty()) {
        return;
    }

    if (d->databaseStatus == KSycocaPrivate::DatabaseNotOpen) {
        return;
    }

    if (d->m_lastCheck.isValid() && d->m_lastCheck.elapsed() <= 1500) {
        return;
    }
    d->m_lastCheck.start();

    // Check if the file on disk was modified since we last checked it.
    QFileInfo info(d->m_databasePath);
    if (info.lastModified() == d->m_dbLastModified) {
        // Check if the watched directories were modified, then the cache needs a rebuild.
        d->checkDirectories(KSycocaPrivate::IfNotFoundOpenDummy);
        return;
    }

    // Close the database and forget all about what we knew.
    // The next call to any public method will recreate
    // everything that's needed.
    d->closeDatabase();
}
