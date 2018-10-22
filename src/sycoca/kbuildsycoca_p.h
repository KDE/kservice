/*  This file is part of the KDE libraries
 *  Copyright (C) 1999 David Faure <faure@kde.org>
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
#ifndef KBUILDSYCOCA_H
#define KBUILDSYCOCA_H

#include "kbuildsycocainterface_p.h"

#include <kservice.h>
#include <ksycoca.h>

#include "vfolder_menu_p.h"

class KBuildServiceGroupFactory;
class QDataStream;
class KCTimeFactory;
class KCTimeDict;

/**
 * @internal
 * Exported for kbuildsycoca, but not installed.
 */
class KSERVICE_EXPORT KBuildSycoca : public KSycoca, public KBuildSycocaInterface
{
    Q_OBJECT
public:
    explicit KBuildSycoca(bool globalDatabase = false);
    ~KBuildSycoca() override;

    /**
     * Recreate the database file.
     * @return true if it was indeed recreated (by us or possibly by someone else), false on error
     */
    bool recreate(bool incremental = true);

    void setTrackId(const QString &id)
    {
        m_trackId = id;
    }

    void setMenuTest(bool b)
    {
        m_menuTest = b;
    }

    QStringList changedResources() const
    {
        return m_changedResources;
    }

    static QStringList factoryResourceDirs();
    static QStringList existingResourceDirs();

    /**
     * Returns a number that identifies the current version of the file @p filename,
     * which is located under GenericDataLocation (including local overrides).
     *
     * When a change is made to the file this number will change.
     */
    static quint32 calcResourceHash(const QString &subdir, const QString &filename);

    /**
     * Compare our current settings (language, prefixes...) with the ones from the existing ksycoca global header.
     * @return true if they match (= we can reuse this ksycoca), false otherwise (full build)
     */
    bool checkGlobalHeader();

    /**
     * @brief path to the sycoca file, for the crash handler in kbuildsycoca
     */
    static const char *sycocaPath();

private:
    /**
     * Add single entry to the sycoca database.
     * Either from a previous database or regenerated from file.
     */
    KSycocaEntry::Ptr createEntry(const QString &file, bool addToFactory);

    /**
     * Implementation of KBuildSycocaInterface
     * Create service and return it. The caller must add it to the servicefactory.
     */
    KService::Ptr createService(const QString &path) override;

    /**
     * Convert a VFolderMenu::SubMenu to KServiceGroups.
     */
    void createMenu(const QString &caption, const QString &name, VFolderMenu::SubMenu *menu);

    /**
     * Build the whole system cache, from .desktop files
     */
    bool build();

    /**
     * Save the ksycoca file
     */
    void save(QDataStream *str);

    /**
     * Clear the factories
     */
    void clear();

    /**
     * @internal
     * @return true if building (i.e. if a KBuildSycoca);
     */
    bool isBuilding() override
    {
        return true;
    }

    QStringList m_changedResources;
    QMap<QString, qint64> m_allResourceDirs; // dir, mtime in ms since epoch
    QString m_trackId;

    QByteArray m_resource; // e.g. "services" (old resource name, now only used for the signal, see kctimefactory.cpp)
    QString m_resourceSubdir; // e.g. "kservices5" (xdgdata subdir)

    KSycocaEntry::List m_tempStorage;
    typedef QList<KSycocaEntry::List> KSycocaEntryListList;
    KSycocaEntryListList *m_allEntries; // entries from existing ksycoca
    KBuildServiceGroupFactory *m_buildServiceGroupFactory = nullptr;
    KSycocaFactory *m_currentFactory = nullptr;
    KCTimeFactory *m_ctimeFactory = nullptr;
    KCTimeDict *m_ctimeDict; // old timestamps
    typedef QHash<QString, KSycocaEntry::Ptr> KBSEntryDict;
    KBSEntryDict *m_currentEntryDict = nullptr;
    KBSEntryDict *m_serviceGroupEntryDict = nullptr;
    VFolderMenu *m_vfolder = nullptr;
    qint64 m_newTimestamp;

    bool m_globalDatabase;
    bool m_menuTest;
    bool m_changed;
};

#endif
