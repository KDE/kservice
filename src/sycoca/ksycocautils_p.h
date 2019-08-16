/*  This file is part of the KDE libraries
 *  Copyright (C) 1999 Waldo Bastian <bastian@kde.org>
 *  Copyright (C) 2005-2013 David Faure <faure@kde.org>
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

#ifndef KSYCOCAUTILS_P_H
#define KSYCOCAUTILS_P_H

#include <QFileInfo>
#include <QString>
#include <QDir>

class QStringList;
class QDataStream;

namespace KSycocaUtilsPrivate
{

// helper function for visitResourceDirectory
template<typename Visitor>
bool visitResourceDirectoryHelper(const QString &dirname, Visitor visitor)
{
    QDir dir(dirname);
    const QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::Unsorted);
    for (const QFileInfo &fi : list) {
        if (fi.isDir() && !fi.isSymLink() && !fi.isBundle()) { // same check as in vfolder_menu.cpp
            if (!visitor(fi)) {
                return false;
            }
            if (!visitResourceDirectoryHelper(fi.filePath(), visitor)) {
                return false;
            }
        }
    }
    return true;
}

// visitor is a function/functor accepts QFileInfo as argument and returns bool
// visitResourceDirectory will visit the resource directory in a depth-first way.
// visitor can terminate the visit by returning false, and visitResourceDirectory
// will also return false in this case, otherwise it will return true.
template<typename Visitor>
bool visitResourceDirectory(const QString &dirname, Visitor visitor)
{
    QFileInfo info(dirname);
    if (!visitor(info)) {
        return false;
    }

    // Recurse only for services and menus.
    // Apps and servicetypes don't need recursion, so save the directory listing.
    if (!dirname.contains(QLatin1String("/applications")) && !dirname.contains(QLatin1String("/kservicetypes5"))) {
        return visitResourceDirectoryHelper(dirname, visitor);
    }

    return true;
}

}

#endif /* KSYCOCAUTILS_P_H */

