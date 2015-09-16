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

#include "ksycocadevices_p.h"
#include "kmemfile_p.h"
#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <fcntl.h>

KSycocaAbstractDevice::~KSycocaAbstractDevice()
{
    delete m_stream;
}

QDataStream *&KSycocaAbstractDevice::stream()
{
    if (!m_stream) {
        m_stream = new QDataStream(device());
        m_stream->setVersion(QDataStream::Qt_3_1);
    }
    return m_stream;
}

#if HAVE_MMAP
KSycocaMmapDevice::KSycocaMmapDevice(const char *sycoca_mmap, size_t sycoca_size)
{
    m_buffer = new QBuffer;
    m_buffer->setData(QByteArray::fromRawData(sycoca_mmap, sycoca_size));
}

KSycocaMmapDevice::~KSycocaMmapDevice()
{
    delete m_buffer;
}

QIODevice *KSycocaMmapDevice::device()
{
    return m_buffer;
}
#endif

KSycocaFileDevice::KSycocaFileDevice(const QString &path)
{
    m_database = new QFile(path);
#ifndef Q_OS_WIN
    (void)fcntl(m_database->handle(), F_SETFD, FD_CLOEXEC);
#endif
}

KSycocaFileDevice::~KSycocaFileDevice()
{
    delete m_database;
}

QIODevice *KSycocaFileDevice::device()
{
    return m_database;
}


#ifndef QT_NO_SHAREDMEMORY
KSycocaMemFileDevice::KSycocaMemFileDevice(const QString &path)
{
    m_database = new KMemFile(path);
}

KSycocaMemFileDevice::~KSycocaMemFileDevice()
{
    delete m_database;
}

QIODevice *KSycocaMemFileDevice::device()
{
    return m_database;
}
#endif

KSycocaBufferDevice::KSycocaBufferDevice()
{
    m_buffer = new QBuffer;
}

KSycocaBufferDevice::~KSycocaBufferDevice()
{
    delete m_buffer;
}

QIODevice *KSycocaBufferDevice::device()
{
    return m_buffer;
}
