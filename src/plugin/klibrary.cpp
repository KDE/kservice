/* This file is part of the KDE libraries
   Copyright (C) 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2000 Michael Matz <matz@kde.org>
   Copyright (C) 2007 Bernhard Loos <nhuh.put@web.de.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "klibrary.h"

#include <QtCore/QDir>
#include <QtCore/QPointer>
#include <QDebug>

#include <kpluginfactory.h>
#include <kpluginloader.h>

// exported for the benefit of KLibLoader
KSERVICE_EXPORT QString findLibrary(const QString &name)
{
    QString libname = KPluginLoader::findPlugin(name);
#ifdef Q_OS_WIN
    // we don't have 'lib' prefix on windows -> remove it and try again
    if (libname.isEmpty()) {
        libname = name;
        QString file, path;

        int pos = libname.lastIndexOf(QLatin1Char('/'));
        if (pos >= 0) {
            file = libname.mid(pos + 1);
            path = libname.left(pos);
            libname = path + QLatin1Char('/') + file.mid(3);
        } else {
            file = libname;
            libname = file.mid(3);
        }
        if (!file.startsWith(QLatin1String("lib"))) {
            return file;
        }

        libname = KPluginLoader::findPlugin(libname);
        if (libname.isEmpty()) {
            libname = name;
        }
    }
#endif
    return libname;
}

KLibrary::KLibrary(QObject *parent)
    : QLibrary(parent), d_ptr(0)
{
}

KLibrary::KLibrary(const QString &name, QObject *parent)
    : QLibrary(findLibrary(name), parent), d_ptr(0)
{
}

KLibrary::KLibrary(const QString &name, int verNum, QObject *parent)
    : QLibrary(findLibrary(name), verNum, parent), d_ptr(0)
{
}

KLibrary::~KLibrary()
{
}

void KLibrary::setFileName(const QString &name)
{
    QLibrary::setFileName(findLibrary(name));
}

