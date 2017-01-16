/*
   This file is part of the KDE libraries
   Copyright (C) 2008 Christian Ehrlicher <ch.ehrlicher@gmx.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KMEMFILE_H
#define KMEMFILE_H

#ifndef QT_NO_SHAREDMEMORY

#include <QtCore/QIODevice>
#include <kservice_export.h>

/**
 * @internal
 * Simple QIODevice for QSharedMemory to keep ksycoca cache in memory only once
 * The first call to open() loads the file into a shm segment. Every
 * subsequent call only attaches to this segment. When the file content changed,
 * you have to execute KMemFile::fileContentsChanged() to update the internal
 * structures. The next call to open() creates a new shm segment. The old one
 * is automatically destroyed when the last process closed KMemFile.
 */

class KMemFile : public QIODevice
{
public:
    /**
     * ctor
     *
     * @param filename the file to load into memory
     * @param parent our parent
     */
    explicit KMemFile(const QString &filename, QObject *parent = nullptr);
    /**
     * dtor
     */
    virtual ~KMemFile();
    /**
     * closes the KMemFile
     *
     * @reimp
     */
    void close() Q_DECL_OVERRIDE;
    /**
     * As KMemFile is a random access device, it returns false
     *
     * @reimp
     */
    bool isSequential() const Q_DECL_OVERRIDE;
    /**
     * @reimp
     * @param mode only QIODevice::ReadOnly is accepted
     */
    bool open(OpenMode mode) Q_DECL_OVERRIDE;
    /**
     * Sets the current read/write position to pos
     * @reimp
     * @param pos the new read/write position
     */
    bool seek(qint64 pos) Q_DECL_OVERRIDE;
    /**
     * Returns the size of the file
     * @reimp
     */
    qint64 size() const Q_DECL_OVERRIDE;
    /**
     * This static function updates the internal information about the file
     * loaded into shared memory. The next time the file is opened, the file is
     * reread from the file system.
     */
    static void fileContentsChanged(const QString &filename);
protected:
    /** @reimp */
    qint64 readData(char *data, qint64 maxSize) Q_DECL_OVERRIDE;
    /** @reimp */
    qint64 writeData(const char *data, qint64 maxSize) Q_DECL_OVERRIDE;
private:
    class Private;
    friend class Private;
    Private *const d;
};

#endif //QT_NO_SHAREDMEMORY

#endif  // KMEMFILE_H
