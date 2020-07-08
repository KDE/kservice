/*  This file is part of the KDE libraries
 *  Copyright (C) 1999 Waldo Bastian <bastian@kde.org>
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

#ifndef KSYCOCAFACTORY_H
#define KSYCOCAFACTORY_H

#include <ksycocaentry.h>
#include <QStandardPaths>

#include <ksycoca.h> // for KSycoca::self()

class QString;
class KSycoca;
class KSycocaDict;
class KSycocaResourceList;
template <typename T> class QList;
template <typename KT, typename VT> class QHash;

typedef QHash<QString, KSycocaEntry::Ptr> KSycocaEntryDict;
class KSycocaFactoryPrivate;
/**
 * @internal
 * Base class for sycoca factories
 * Exported for unit tests
 */
class KSERVICE_EXPORT KSycocaFactory
{
public:
    virtual KSycocaFactoryId factoryId() const = 0;

protected: // virtual class
    /**
     * Create a factory which can be used to lookup from/create a database
     * (depending on KSycoca::isBuilding())
     */
    explicit KSycocaFactory(KSycocaFactoryId factory_id, KSycoca *sycoca);

public:
    virtual ~KSycocaFactory();

    /**
     * @return the position of the factory in the sycoca file
     */
    int offset() const;

    /**
     * @return the dict, for special use by KBuildSycoca
     */
    KSycocaEntryDict *entryDict()
    {
        return m_entryDict;
    }

    /**
     * Construct an entry from a config file.
     * To be implemented in the real factories.
     */
    virtual KSycocaEntry *createEntry(const QString &file) const = 0;

    /**
     * Add an entry
     */
    virtual void addEntry(const KSycocaEntry::Ptr &newEntry);

    /**
     * Remove all entries with the given name.
     * Not very fast (O(N)), use with care.
     */
    void removeEntry(const QString &entryName);

    /**
     * Read an entry from the database
     */
    virtual KSycocaEntry *createEntry(int offset) const = 0;

    /**
     * Get a list of all entries from the database.
     */
    virtual KSycocaEntry::List allEntries() const;

    /**
     * Saves all entries it maintains as well as index files
     * for these entries to the stream 'str'.
     *
     * Also sets mOffset to the starting position.
     *
     * The stream is positioned at the end of the last index.
     *
     * Don't forget to call the parent first when you override
     * this function.
     */
    virtual void save(QDataStream &str);

    /**
     * Writes out a header to the stream 'str'.
     * The baseclass positions the stream correctly.
     *
     * Don't forget to call the parent first when you override
     * this function.
     */
    virtual void saveHeader(QDataStream &str);

    /**
     * @return the resources for which this factory is responsible.
     * @internal to kbuildsycoca
     */
    const KSycocaResourceList *resourceList() const;

    /**
     * @return the sycoca dict, for factories to find entries by name.
     */
    const KSycocaDict *sycocaDict() const;

    /**
     * @return true if the factory is completely empty - no entries defined
     */
    bool isEmpty() const;

    KSycoca *sycoca() const { return m_sycoca; }

protected:
    QDataStream *stream() const;

    KSycocaResourceList *m_resourceList = nullptr;
    KSycocaEntryDict *m_entryDict = nullptr;

    /**
     * Returns all directories for the given @p subdir of GenericDataLocation.
     * Helper function for AnyFactory::resourceDirs().
     */
    static QStringList allDirectories(const QString &subdir);

private:
    QDataStream *m_str = nullptr;
    KSycoca *m_sycoca = nullptr;
    KSycocaFactoryPrivate *const d;

protected:
    /** Virtual hook, used to add new "virtual" functions while maintaining
        binary compatibility. Unused in this class.
    */
    virtual void virtual_hook(int id, void *data);
};

/**
 * This, instead of a typedef, allows to declare "class ..." in header files.
 * @internal
 */
class KSycocaFactoryList : public QList<KSycocaFactory *> //krazy:exclude=dpointer (acts as a typedef)
{
public:
    KSycocaFactoryList() { }
};

#endif
