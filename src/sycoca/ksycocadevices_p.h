/* This file is part of the KDE project

   Copyright 2009 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KSYCOCADEVICES_P_H
#define KSYCOCADEVICES_P_H

#include <config-ksycoca.h>
#include <stdlib.h>
// TODO: remove mmap() from kdewin32 and use QFile::mmap() when needed
#ifdef Q_OS_WIN
#undef HAVE_MMAP
#endif

class QString;
class QDataStream;
class QBuffer;
class QFile;
class QIODevice;
class KMemFile;

class KSycocaAbstractDevice
{
public:
    KSycocaAbstractDevice() : m_stream(nullptr)
    {
    }

    virtual ~KSycocaAbstractDevice();

    virtual QIODevice *device() = 0;

    QDataStream *&stream();

private:
    QDataStream *m_stream;
};

#if HAVE_MMAP
// Reading from a mmap'ed file
class KSycocaMmapDevice : public KSycocaAbstractDevice
{
public:
    KSycocaMmapDevice(const char *sycoca_mmap, size_t sycoca_size);
    ~KSycocaMmapDevice() override;
    QIODevice *device() override;
private:
    QBuffer *m_buffer;
};
#endif

// Reading from a QFile
class KSycocaFileDevice : public KSycocaAbstractDevice
{
public:
    explicit KSycocaFileDevice(const QString &path);
    ~KSycocaFileDevice() override;
    QIODevice *device() override;
private:
    QFile *m_database;
};

#ifndef QT_NO_SHAREDMEMORY
// Reading from a KMemFile
class KSycocaMemFileDevice : public KSycocaAbstractDevice
{
public:
    explicit KSycocaMemFileDevice(const QString &path);
    ~KSycocaMemFileDevice() override;
    QIODevice *device() override;
private:
    KMemFile *m_database;
};
#endif

// Reading from a dummy memory buffer
class KSycocaBufferDevice : public KSycocaAbstractDevice
{
public:
    KSycocaBufferDevice();
    ~KSycocaBufferDevice() override;
    QIODevice *device() override;
private:
    QBuffer *m_buffer;
};

#endif /* KSYCOCADEVICES_P_H */
