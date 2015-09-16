/*  This file is part of the KDE libraries
 *  Copyright (C) 1999 David Faure <faure@kde.org>
 *  Copyright (C) 2002-2003 Waldo Bastian <bastian@kde.org>
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

#include "kbuildsycoca.h"
#include "ksycoca_p.h"
#include "ksycocaresourcelist.h"
#include "vfolder_menu.h"

#include <kservice.h>
#include "kbuildservicetypefactory.h"
#include "kbuildmimetypefactory.h"
#include "kbuildservicefactory.h"
#include "kbuildservicegroupfactory.h"
#include "kctimefactory.h"
#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QLocale>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusConnectionInterface>
#include <QDirIterator>
#include <QDateTime>
#include <qsavefile.h>

#include <kcrash.h>
#include <kmemfile_p.h>
#include <klocalizedstring.h>
#include <kaboutdata.h>

#include <qplatformdefs.h>
#include <time.h>
#include <memory> // auto_ptr
#include <qstandardpaths.h>
#include <qcommandlineparser.h>
#include <qcommandlineoption.h>

#include "../../kservice_version.h"

static qint64 newTimestamp = 0;

static bool bGlobalDatabase = false;
static bool bMenuTest = false;

static const char *s_cSycocaPath = 0;
static void crashHandler(int)
{
    // If we crash while reading sycoca, we delete the database
    // in an attempt to recover.
    if (s_cSycocaPath) {
        unlink(s_cSycocaPath);
    }
}

static QString sycocaPath()
{
    QFileInfo fi(KSycoca::absoluteFilePath(bGlobalDatabase ? KSycoca::GlobalDatabase : KSycoca::LocalDatabase));
    if (!QDir().mkpath(fi.absolutePath())) {
        qWarning() << "Couldn't create" << fi.absolutePath();
    }
    return fi.absoluteFilePath();
}

KBuildSycocaInterface::~KBuildSycocaInterface() {}

KBuildSycoca::KBuildSycoca()
    : KSycoca(true),
      m_allEntries(0),
      m_serviceFactory(0),
      m_buildServiceGroupFactory(0),
      m_currentFactory(0),
      m_ctimeFactory(0),
      m_ctimeDict(0),
      m_currentEntryDict(0),
      m_serviceGroupEntryDict(0),
      m_vfolder(0),
      m_changed(false)
{
}

KBuildSycoca::~KBuildSycoca()
{
    // Delete the factories while we exist, so that the virtual isBuilding() still works
    qDeleteAll(*factories());
    factories()->clear();
}

KSycocaEntry::Ptr KBuildSycoca::createEntry(const QString &file, bool addToFactory)
{
    quint32 timeStamp = m_ctimeFactory->dict()->ctime(file, m_resource);
    if (!timeStamp) {
        timeStamp = calcResourceHash(m_resourceSubdir, file);
    }
    KSycocaEntry::Ptr entry;
    if (m_allEntries) {
        Q_ASSERT(m_ctimeDict);
        quint32 oldTimestamp = m_ctimeDict->ctime(file, m_resource);
        if (file.contains("fake")) {
            qDebug() << "m_ctimeDict->ctime(" << file << ") = " << oldTimestamp << "compared with" << timeStamp;
        }

        if (timeStamp && (timeStamp == oldTimestamp)) {
            // Re-use old entry
            if (m_currentFactory == m_buildServiceGroupFactory) { // Strip .directory from service-group entries
                entry = m_currentEntryDict->value(file.left(file.length() - 10));
            } else {
                entry = m_currentEntryDict->value(file);
            }
            // remove from m_ctimeDict; if m_ctimeDict is not empty
            // after all files have been processed, it means
            // some files were removed since last time
            if (file.contains("fake")) {
                qDebug() << "reusing (and removing) old entry for:" << file << "entry=" << entry;
            }
            m_ctimeDict->remove(file, m_resource);
        } else if (oldTimestamp) {
            m_changed = true;
            m_ctimeDict->remove(file, m_resource);
            qDebug() << "modified:" << file;
        } else {
            m_changed = true;
            qDebug() << "new:" << file;
        }
    }
    m_ctimeFactory->dict()->addCTime(file, m_resource, timeStamp);
    if (!entry) {
        // Create a new entry
        entry = m_currentFactory->createEntry(file);
    }
    if (entry && entry->isValid()) {
        if (addToFactory) {
            m_currentFactory->addEntry(entry);
        } else {
            m_tempStorage.append(entry);
        }
        return entry;
    }
    return KSycocaEntry::Ptr();
}

KService::Ptr KBuildSycoca::createService(const QString &path)
{
    KSycocaEntry::Ptr entry = createEntry(path, false);
    return KService::Ptr(static_cast<KService*>(entry.data()));
}

// returns false if the database is up to date, true if it needs to be saved
bool KBuildSycoca::build()
{
    typedef QLinkedList<KBSEntryDict *> KBSEntryDictList;
    KBSEntryDictList entryDictList;
    KBSEntryDict *serviceEntryDict = 0;

    // Convert for each factory the entryList to a Dict.
    int i = 0;
    // For each factory
    for (KSycocaFactoryList::Iterator factory = factories()->begin();
            factory != factories()->end();
            ++factory) {
        KBSEntryDict *entryDict = new KBSEntryDict;
        if (m_allEntries) {
            const KSycocaEntry::List list = (*m_allEntries)[i++];
            Q_FOREACH (const KSycocaEntry::Ptr &entry, list) {
                //if (entry->entryPath().contains("fake"))
                //    qDebug() << "inserting into entryDict:" << entry->entryPath() << entry;
                entryDict->insert(entry->entryPath(), entry);
            }
        }
        if ((*factory) == m_serviceFactory) {
            serviceEntryDict = entryDict;
        } else if ((*factory) == m_buildServiceGroupFactory) {
            m_serviceGroupEntryDict = entryDict;
        }
        entryDictList.append(entryDict);
    }

    QMap<QString, QByteArray> allResourcesSubDirs; // dirs, kstandarddirs-resource-name
    // For each factory
    for (KSycocaFactoryList::Iterator factory = factories()->begin();
            factory != factories()->end();
            ++factory) {
        // For each resource the factory deals with
        const KSycocaResourceList *list = (*factory)->resourceList();
        if (!list) {
            continue;
        }
        Q_FOREACH (const KSycocaResource &res, *list) {
            // With this we would get dirs, but not a unique list of relative files (for global+local merging to work)
            //const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, res.subdir, QStandardPaths::LocateDirectory);
            //allResourcesSubDirs[res.resource] += dirs;
            allResourcesSubDirs.insert(res.subdir, res.resource);
        }
    }

    m_ctimeFactory = new KCTimeFactory(this); // This is a build factory too, don't delete!!
    bool uptodate = true;
    for (QMap<QString, QByteArray>::ConstIterator it1 = allResourcesSubDirs.constBegin();
            it1 != allResourcesSubDirs.constEnd();
            ++it1) {
        m_changed = false;
        m_resourceSubdir = it1.key();
        m_resource = it1.value();

        QStringList relFiles;
        const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, m_resourceSubdir, QStandardPaths::LocateDirectory);
        Q_FOREACH (const QString &dir, dirs) {
            QDirIterator it(dir, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString filePath = it.next();
                Q_ASSERT(filePath.startsWith(dir)); // due to the line below...
                const QString relPath = filePath.mid(dir.length() + 1);
                if (!relFiles.contains(relPath)) {
                    relFiles.append(relPath);
                }
            }
        }
        // Now find all factories that use this resource....
        // For each factory
        KBSEntryDictList::const_iterator ed_it = entryDictList.begin();
        const KBSEntryDictList::const_iterator ed_end = entryDictList.end();
        KSycocaFactoryList::const_iterator it = factories()->constBegin();
        const KSycocaFactoryList::const_iterator end = factories()->constEnd();
        for (; it != end; ++it, ++ed_it) {
            m_currentFactory = (*it);
            // m_ctimeInfo gets created after the initial loop, so it has no entryDict.
            m_currentEntryDict = ed_it == ed_end ? 0 : *ed_it;
            // For each resource the factory deals with
            const KSycocaResourceList *list = m_currentFactory->resourceList();
            if (!list) {
                continue;
            }

            for (KSycocaResourceList::ConstIterator it2 = list->constBegin();
                    it2 != list->constEnd();
                    ++it2) {
                KSycocaResource res = (*it2);
                if (res.resource != (*it1)) {
                    continue;
                }

                // For each file in the resource
                for (QStringList::ConstIterator it3 = relFiles.constBegin();
                        it3 != relFiles.constEnd();
                        ++it3) {
                    // Check if file matches filter
                    if ((*it3).endsWith(res.extension)) {
                        QString entryPath = (*it3);
                        createEntry(entryPath, true);
                    }
                }
            }
        }
        if (m_changed || !m_allEntries) {
            uptodate = false;
            //qDebug() << "CHANGED:" << resource;
            m_changedResources.append(m_resource);
        }
    }

    bool result = !uptodate || (m_ctimeDict && !m_ctimeDict->isEmpty());
    if (m_ctimeDict && !m_ctimeDict->isEmpty()) {
        //qDebug() << "Still in time dict:";
        //m_ctimeDict->dump();
        // ## It seems entries filtered out by vfolder are still in there,
        // so we end up always saving ksycoca, i.e. this method never returns false

        // Get the list of resources from which some files were deleted
        const QStringList resources = m_ctimeDict->remainingResourceList();
        qDebug() << "Still in the time dict (i.e. deleted files)" << resources;
        m_changedResources += resources;
    }

    if (result || bMenuTest) {
        m_resource = "apps";
        m_resourceSubdir = QStringLiteral("applications");
        m_currentFactory = m_serviceFactory;
        m_currentEntryDict = serviceEntryDict;
        m_changed = false;

        m_vfolder = new VFolderMenu(m_serviceFactory, this);
        if (!m_trackId.isEmpty()) {
            m_vfolder->setTrackId(m_trackId);
        }

        VFolderMenu::SubMenu *kdeMenu = m_vfolder->parseMenu(QStringLiteral("applications.menu"));

        KServiceGroup::Ptr entry = m_buildServiceGroupFactory->addNew(QStringLiteral("/"), kdeMenu->directoryFile, KServiceGroup::Ptr(), false);
        entry->setLayoutInfo(kdeMenu->layoutList);
        createMenu(QString(), QString(), kdeMenu);

        m_allResourceDirs = factoryResourceDirs();
        m_allResourceDirs += m_vfolder->allDirectories();

        if (m_changed || !m_allEntries) {
            uptodate = false;
            //qDebug() << "CHANGED:" << m_resource;
            m_changedResources.append(m_resource);
        }
        if (bMenuTest) {
            result = false;
        }
    }

    qDeleteAll(entryDictList);
    return result;
}

void KBuildSycoca::createMenu(const QString &caption_, const QString &name_, VFolderMenu::SubMenu *menu)
{
    QString caption = caption_;
    QString name = name_;
    foreach (VFolderMenu::SubMenu *subMenu, menu->subMenus) {
        QString subName = name + subMenu->name + '/';

        QString directoryFile = subMenu->directoryFile;
        if (directoryFile.isEmpty()) {
            directoryFile = subName + QStringLiteral(".directory");
        }
        quint32 timeStamp = m_ctimeFactory->dict()->ctime(directoryFile, m_resource);
        if (!timeStamp) {
            timeStamp = calcResourceHash(m_resourceSubdir, directoryFile);
        }

        KServiceGroup::Ptr entry;
        if (m_allEntries) {
            const quint32 oldTimestamp = m_ctimeDict->ctime(directoryFile, m_resource);

            if (timeStamp && (timeStamp == oldTimestamp)) {
                KSycocaEntry::Ptr group = m_serviceGroupEntryDict->value(subName);
                if (group) {
                    entry = KServiceGroup::Ptr(static_cast<KServiceGroup*>(group.data()));
                    if (entry->directoryEntryPath() != directoryFile) {
                        entry = 0;    // Can't reuse this one!
                    }
                }
            }
        }
        if (timeStamp) { // bug? (see calcResourceHash). There might not be a .directory file...
            m_ctimeFactory->dict()->addCTime(directoryFile, m_resource, timeStamp);
        }

        entry = m_buildServiceGroupFactory->addNew(subName, subMenu->directoryFile, entry, subMenu->isDeleted);
        entry->setLayoutInfo(subMenu->layoutList);
        if (!(bMenuTest && entry->noDisplay())) {
            createMenu(caption + entry->caption() + '/', subName, subMenu);
        }
    }
    if (caption.isEmpty()) {
        caption += '/';
    }
    if (name.isEmpty()) {
        name += '/';
    }
    foreach (const KService::Ptr &p, menu->items) {
        if (bMenuTest) {
            if (!menu->isDeleted && !p->noDisplay())
                printf("%s\t%s\t%s\n", qPrintable(caption), qPrintable(p->menuId()),
                       qPrintable(QStandardPaths::locate(QStandardPaths::ApplicationsLocation, p->entryPath())));
        } else {
            m_buildServiceGroupFactory->addNewEntryTo(name, p);
        }
    }
}

bool KBuildSycoca::recreate(bool incremental)
{
    QString path(sycocaPath());

    QByteArray qSycocaPath = QFile::encodeName(path);
    s_cSycocaPath = qSycocaPath.data();

    m_allEntries = 0;
    m_ctimeDict = 0;
    if (incremental) {
        qDebug() << "Reusing existing ksycoca";
        KSycoca *oldSycoca = KSycoca::self();
        m_allEntries = new KSycocaEntryListList;
        m_ctimeDict = new KCTimeDict;

        // Must be in same order as in KBuildSycoca::recreate()!
        m_allEntries->append(KServiceTypeFactory::self()->allEntries());
        m_allEntries->append(KMimeTypeFactory::self()->allEntries());
        m_allEntries->append(KServiceGroupFactory::self()->allEntries());
        m_allEntries->append(KServiceFactory::self()->allEntries());

        KCTimeFactory *ctimeInfo = new KCTimeFactory(oldSycoca);
        *m_ctimeDict = ctimeInfo->loadDict();
    }
    s_cSycocaPath = 0;

    QSaveFile database(path);
    bool openedOK = database.open(QIODevice::WriteOnly);

    if (!openedOK && database.error() == QFile::WriteError && QFile::exists(path)) {
        QFile::remove(path);
        openedOK = database.open(QIODevice::WriteOnly);
    }
    if (!openedOK) {
        fprintf(stderr, KBUILDSYCOCA_EXENAME ": ERROR creating database '%s'! %s\n",
                path.toLocal8Bit().data(), database.errorString().toLocal8Bit().data());
        return false;
    }

    QDataStream *str = new QDataStream(&database);
    str->setVersion(QDataStream::Qt_3_1);

    qDebug().nospace() << "Recreating ksycoca file (" << path << ", version " << KSycoca::version() << ")";

    // It is very important to build the servicetype one first
    KBuildServiceTypeFactory *stf = new KBuildServiceTypeFactory(this);
    KBuildMimeTypeFactory *mimeTypeFactory = new KBuildMimeTypeFactory(this);
    m_buildServiceGroupFactory = new KBuildServiceGroupFactory(this);
    m_serviceFactory = new KBuildServiceFactory(stf, mimeTypeFactory, m_buildServiceGroupFactory);

    if (build()) { // Parse dirs
        save(str); // Save database
        if (str->status() != QDataStream::Ok) { // Probably unnecessary now in Qt5, since QSaveFile detects write errors
            database.cancelWriting();    // Error
        }
        delete str;
        str = 0;

        //if we are currently via sudo, preserve the original owner
        //as $HOME may also be that of another user rather than /root
#ifdef Q_OS_UNIX
        if (qEnvironmentVariableIsSet("SUDO_UID")) {
            const int uid = QString(qgetenv("SUDO_UID")).toInt();
            const int gid = QString(qgetenv("SUDO_GID")).toInt();
            if (uid && gid) {
                fchown(database.handle(), uid, gid);
            }
        }
#endif

        if (!database.commit()) {
            fprintf(stderr, KBUILDSYCOCA_EXENAME ": ERROR writing database '%s'!\n", database.fileName().toLocal8Bit().data());
            fprintf(stderr, KBUILDSYCOCA_EXENAME ": Disk full?\n");
            return false;
        }
    } else {
        delete str;
        str = 0;
        database.cancelWriting();
        if (bMenuTest) {
            return true;
        }
        qDebug() << "Database is up to date";
    }

    if (!bGlobalDatabase) {
        // update the timestamp file
        QString stamppath = path + QStringLiteral("stamp");
        QFile ksycocastamp(stamppath);
        ksycocastamp.open(QIODevice::WriteOnly);
        QDataStream str(&ksycocastamp);
        str.setVersion(QDataStream::Qt_5_3);
        str << newTimestamp;
        str << existingResourceDirs();
        if (m_vfolder) {
            str << m_vfolder->allDirectories();    // Extra resource dirs
        }
    }
    if (d->m_sycocaStrategy == KSycocaPrivate::StrategyMemFile) {
        KMemFile::fileContentsChanged(path);
    }

    return true;
}

void KBuildSycoca::save(QDataStream *str)
{
    // Write header (#pass 1)
    str->device()->seek(0);

    (*str) << qint32(KSycoca::version());
    //KSycocaFactory * servicetypeFactory = 0;
    //KBuildMimeTypeFactory * mimeTypeFactory = 0;
    KBuildServiceFactory *serviceFactory = 0;
    for (KSycocaFactoryList::Iterator factory = factories()->begin();
            factory != factories()->end();
            ++factory) {
        qint32 aId;
        qint32 aOffset;
        aId = (*factory)->factoryId();
        //if ( aId == KST_KServiceTypeFactory )
        //   servicetypeFactory = *factory;
        //else if ( aId == KST_KMimeTypeFactory )
        //   mimeTypeFactory = static_cast<KBuildMimeTypeFactory *>( *factory );
        if (aId == KST_KServiceFactory) {
            serviceFactory = static_cast<KBuildServiceFactory *>(*factory);
        }
        aOffset = (*factory)->offset(); // not set yet, so always 0
        (*str) << aId;
        (*str) << aOffset;
    }
    (*str) << qint32(0); // No more factories.
    // Write XDG_DATA_DIRS
    (*str) << QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).join(QString(QLatin1Char(':')));
    (*str) << (quint32)newTimestamp / 1000; // TODO just newTimestamp when using a new filename
    (*str) << QLocale().bcp47Name();
    // This makes it possible to trigger a ksycoca update for all users (KIOSK feature)
    (*str) << calcResourceHash(QStringLiteral("kservices5"), QStringLiteral("update_ksycoca"));
    (*str) << m_allResourceDirs;

    // Calculate per-servicetype/mimetype data
    if (serviceFactory) serviceFactory->postProcessServices();

    // Here so that it's the last debug message
    qDebug() << "Saving";

    // Write factory data....
    for (KSycocaFactoryList::Iterator factory = factories()->begin();
            factory != factories()->end();
            ++factory) {
        (*factory)->save(*str);
        if (str->status() != QDataStream::Ok) { // ######## TODO: does this detect write errors, e.g. disk full?
            return;    // error
        }
    }

    int endOfData = str->device()->pos();

    // Write header (#pass 2)
    str->device()->seek(0);

    (*str) << qint32(KSycoca::version());
    for (KSycocaFactoryList::Iterator factory = factories()->begin();
            factory != factories()->end(); ++factory) {
        qint32 aId;
        qint32 aOffset;
        aId = (*factory)->factoryId();
        aOffset = (*factory)->offset();
        (*str) << aId;
        (*str) << aOffset;
    }
    (*str) << qint32(0); // No more factories.

    // Jump to end of database
    str->device()->seek(endOfData);
}

static bool checkDirTimestamps(const QString &dirname, const QDateTime &stamp)
{
    QDir dir(dirname);
    const QFileInfoList list = dir.entryInfoList(QDir::NoFilter, QDir::Unsorted);
    if (list.isEmpty()) {
        return true;
    }

    foreach (const QFileInfo &fi, list) {
        if (fi.fileName() == "." || fi.fileName() == "..") {
            continue;
        }
        if (fi.lastModified() > stamp) {
            qDebug() << "timestamp changed:" << fi.filePath() << fi.lastModified() << ">" << stamp;
            return false;
        }
        if (fi.isDir() && !checkDirTimestamps(fi.filePath(), stamp)) {
            return false;
        }
    }
    return true;
}

// check times of last modification of all files on which ksycoca depends,
// and also their directories
// if all of them are older than the timestamp in file ksycocastamp, this
// means that there's no need to rebuild ksycoca
static bool checkTimestamps(qint64 timestamp, const QStringList &dirs)
{
    qDebug() << "checking file timestamps";
    const QDateTime stamp = QDateTime::fromMSecsSinceEpoch(timestamp);
    for (QStringList::ConstIterator it = dirs.begin();
            it != dirs.end();
            ++it) {
        QFileInfo inf(*it);
        if (inf.lastModified() > stamp) {
            qDebug() << "timestamp changed:" << *it;
            return false;
        }
        if (!checkDirTimestamps(*it, stamp)) {
            return false;
        }
    }
    qDebug() << "timestamps check ok";
    return true;
}

QStringList KBuildSycoca::factoryResourceDirs()
{
    static QStringList *dirs = NULL;
    if (dirs != NULL) {
        return *dirs;
    }
    dirs = new QStringList;
    // these are all resource dirs cached by ksycoca
    *dirs += KServiceTypeFactory::resourceDirs();
    *dirs += KMimeTypeFactory::resourceDirs();
    *dirs += KServiceFactory::resourceDirs();

    return *dirs;
}

QStringList KBuildSycoca::existingResourceDirs()
{
    static QStringList *dirs = NULL;
    if (dirs != NULL) {
        return *dirs;
    }
    dirs = new QStringList(factoryResourceDirs());

    for (QStringList::Iterator it = dirs->begin();
            it != dirs->end();) {
        QFileInfo inf(*it);
        if (!inf.exists() || !inf.isReadable()) {
            it = dirs->erase(it);
        } else {
            ++it;
        }
    }
    return *dirs;
}

static const char appFullName[] = "org.kde.kbuildsycoca";

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kservice5");

    KAboutData about(KBUILDSYCOCA_EXENAME,
                     i18nc("application name", "KBuildSycoca"),
                     QStringLiteral(KSERVICE_VERSION_STRING),
                     i18nc("application description", "Rebuilds the system configuration cache."),
                     KAboutLicense::GPL,
                     i18nc("@info:credit", "Copyright 1999-2014 KDE Developers"));
    about.addAuthor(i18nc("@info:credit", "David Faure"),
                    i18nc("@info:credit", "Author"),
                    QStringLiteral("faure@kde.org"));
    about.addAuthor(i18nc("@info:credit", "Waldo Bastian"),
                    i18nc("@info:credit", "Author"),
                    QStringLiteral("bastian@kde.org"));
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringLiteral("nosignal"),
                i18nc("@info:shell command-line option",
                      "Do not signal applications to update")));
    parser.addOption(QCommandLineOption(QStringLiteral("noincremental"),
                i18nc("@info:shell command-line option",
                      "Disable incremental update, re-read everything")));
    parser.addOption(QCommandLineOption(QStringLiteral("checkstamps"),
                i18nc("@info:shell command-line option",
                      "Check file timestamps")));
    parser.addOption(QCommandLineOption(QStringLiteral("nocheckfiles"),
                i18nc("@info:shell command-line option",
                      "Disable checking files (dangerous)")));
    parser.addOption(QCommandLineOption(QStringLiteral("global"),
                i18nc("@info:shell command-line option",
                      "Create global database")));
    parser.addOption(QCommandLineOption(QStringLiteral("menutest"),
                i18nc("@info:shell command-line option",
                      "Perform menu generation test run only")));
    parser.addOption(QCommandLineOption(QStringLiteral("track"),
                i18nc("@info:shell command-line option",
                      "Track menu id for debug purposes"),
                QStringLiteral("menu-id")));
    parser.addOption(QCommandLineOption(QStringLiteral("testmode"),
                i18nc("@info:shell command-line option",
                      "Switch QStandardPaths to test mode, for unit tests only")));
    parser.process(app);
    about.processCommandLine(&parser);

    bGlobalDatabase = parser.isSet(QStringLiteral("global"));
    bMenuTest = parser.isSet(QStringLiteral("menutest"));

    if (parser.isSet(QStringLiteral("testmode"))) {
        QStandardPaths::enableTestMode(true);
    }

    if (bGlobalDatabase) {
        // Qt uses XDG_DATA_HOME as first choice for GenericDataLocation so we set it to 2nd entry
        QStringList paths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
        if (paths.size() >= 2) {
            qputenv("XDG_DATA_HOME", paths.at(1).toLocal8Bit());
        }
    }

    KCrash::setEmergencySaveFunction(crashHandler);

    while (QDBusConnection::sessionBus().isConnected()) {
        // Detect already-running kbuildsycoca instances using DBus.
        if (QDBusConnection::sessionBus().interface()->registerService(appFullName, QDBusConnectionInterface::QueueService)
                != QDBusConnectionInterface::ServiceQueued) {
            break; // Go
        }
        fprintf(stderr, "Waiting for already running %s to finish.\n", KBUILDSYCOCA_EXENAME);

        QEventLoop eventLoop;
        QObject::connect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceRegistered(QString)),
                         &eventLoop, SLOT(quit()));
        eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
    }
    fprintf(stderr, "%s running...\n", KBUILDSYCOCA_EXENAME);

    bool checkfiles = bGlobalDatabase || !parser.isSet(QStringLiteral("nocheckfiles"));

    bool incremental = !bGlobalDatabase && !parser.isSet(QStringLiteral("noincremental")) && checkfiles;
    if (incremental || !checkfiles) {
        KSycoca::disableAutoRebuild(); // Prevent deadlock
        if (!KBuildSycoca::checkGlobalHeader()) {
            incremental = false;
            checkfiles = true;
            KBuildSycoca::clearCaches();
        }
    }

    bool checkstamps = incremental && parser.isSet(QStringLiteral("checkstamps")) && checkfiles;
    qint64 filestamp = 0;
    QStringList oldresourcedirs;
    if (checkstamps && incremental) {
        QString path = sycocaPath() + QStringLiteral("stamp");
        QByteArray qPath = QFile::encodeName(path);
        s_cSycocaPath = qPath.data(); // Delete timestamps on crash
        QFile ksycocastamp(path);
        if (ksycocastamp.open(QIODevice::ReadOnly)) {
            QDataStream str(&ksycocastamp);
            str.setVersion(QDataStream::Qt_5_3);

            if (!str.atEnd()) {
                str >> filestamp;
            }
            if (!str.atEnd()) {
                str >> oldresourcedirs;
                if (oldresourcedirs != KBuildSycoca::existingResourceDirs()) {
                    checkstamps = false;
                }
            } else {
                checkstamps = false;
            }
            if (!str.atEnd()) {
                QStringList extraResourceDirs;
                str >> extraResourceDirs;
                oldresourcedirs += extraResourceDirs;
            }
        } else {
            checkstamps = false;
        }
        s_cSycocaPath = 0;
    }

    newTimestamp = QDateTime::currentMSecsSinceEpoch();
    QStringList changedResources;

    if (checkfiles && (!checkstamps || !checkTimestamps(filestamp, oldresourcedirs))) {

        KBuildSycoca sycoca; // Build data base
        if (parser.isSet(QStringLiteral("track"))) {
            sycoca.setTrackId(parser.value(QStringLiteral("track")));
        }
        if (!sycoca.recreate(incremental)) {
            return -1;
        }
        changedResources = sycoca.changedResources();

        if (bGlobalDatabase) {
            // These directories may have been created with 0700 permission
            // better delete them if they are empty
            QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
            QDir().remove(appsDir);
            // was doing the same with servicetypes, but I don't think any of these gets created-by-mistake anymore.
        }
    }

    if (!parser.isSet(QStringLiteral("nosignal"))) {
        // Notify ALL applications that have a ksycoca object, using a signal
        QDBusMessage signal = QDBusMessage::createSignal("/", "org.kde.KSycoca", "notifyDatabaseChanged");
        signal << changedResources;

        if (QDBusConnection::sessionBus().isConnected()) {
            qDebug() << "Emitting notifyDatabaseChanged" << changedResources;
            QDBusConnection::sessionBus().send(signal);
            qApp->processEvents(); // make sure the dbus signal is sent before we quit.
        }
    }

    return 0;
}

static quint32 updateHash(const QString &file, quint32 hash)
{
    QFileInfo fi(file);
    if (fi.isReadable() && fi.isFile()) {
        // This was using buff.st_ctime (in Waldo's initial commit to kstandarddirs.cpp in 2001), but that looks wrong?
        // Surely we want to catch manual editing, while a chmod doesn't matter much?
        hash += fi.lastModified().toTime_t();
    }
    return hash;
}

quint32 KBuildSycoca::calcResourceHash(const QString &resourceSubDir, const QString &filename)
{
    quint32 hash = 0;
    if (!QDir::isRelativePath(filename)) {
        return updateHash(filename, hash);
    }
    const QStringList files = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, resourceSubDir + QLatin1Char('/') + filename);
    Q_FOREACH (const QString &file, files) {
        hash = updateHash(file, hash);
    }
    if (hash == 0 && !filename.endsWith(QLatin1String("update_ksycoca"))
            && !filename.endsWith(QLatin1String(".directory")) // bug? needs investigation from someone who understands the VFolder spec
       ) {
        qWarning() << "File not found or not readable:" << filename << "found:" << files;
        Q_ASSERT(hash != 0);
    }
    return hash;
}

bool KBuildSycoca::checkGlobalHeader()
{
    const QString current_language = QLocale().bcp47Name();
    const quint32 current_update_sig = KBuildSycoca::calcResourceHash(QStringLiteral("kservices5"), QStringLiteral("update_ksycoca"));
    const QString current_prefixes = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).join(QString(QLatin1Char(':')));

    const KSycocaHeader header = KSycoca::self()->readSycocaHeader();
    Q_ASSERT(!header.prefixes.split(':').contains(QDir::homePath()));

    return (current_update_sig == header.updateSignature) &&
            (current_language == header.language) &&
            (current_prefixes == header.prefixes) &&
            (header.timeStamp != 0);
}
