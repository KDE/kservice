/* This file is part of KDE Frameworks

   Copyright 2018 David Faure <faure@kde.org>

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

#ifndef SETUP_XDG_DIRS_H
#define SETUP_XDG_DIRS_H

#include <QStandardPaths>
#include <QFile>
#include <QCoreApplication>
#include <QDebug>

static void setupXdgDirs()
{
    const QByteArray defaultDataDirs = qEnvironmentVariableIsSet("XDG_DATA_DIRS") ? qgetenv("XDG_DATA_DIRS") : QByteArray("/usr/local/share:/usr/share");
    const QByteArray modifiedDataDirs = QFile::encodeName(QCoreApplication::applicationDirPath()) + "/data:" + defaultDataDirs;
    qputenv("XDG_DATA_DIRS", modifiedDataDirs);

    const QByteArray defaultConfigDirs = qEnvironmentVariableIsSet("XDG_CONFIG_DIRS") ? qgetenv("XDG_CONFIG_DIRS") : QByteArray("/etc/xdg");
    const QByteArray modifiedConfigDirs = QFile::encodeName(QCoreApplication::applicationDirPath()) + "/data:" + defaultConfigDirs;
    qputenv("XDG_CONFIG_DIRS", modifiedConfigDirs);
}

#endif
