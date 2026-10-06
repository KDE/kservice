/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2005-2013 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KSYCOCAUTILS_P_H
#define KSYCOCAUTILS_P_H

#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QString>

namespace KSycocaUtilsPrivate
{
// QMimeDatabase checks its files for changes at most every few seconds. A new database of KSycoca
// can come with new MIME types, which a build reads and the lookups that follow resolve.
inline void reloadMimeDatabase()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 13, 0)
    QMimeDatabase::reload();
#endif
}

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
    if (!dirname.contains(QLatin1String("/applications"))) {
        return visitResourceDirectoryHelper(dirname, visitor);
    }

    return true;
}

}

#endif /* KSYCOCAUTILS_P_H */
