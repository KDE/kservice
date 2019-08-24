/* This file is part of the KDE project
   Copyright (C) 2015 Gregor Mi <codestruct@posteo.org>

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

#ifndef KSERVICEUTIL_P_H
#define KSERVICEUTIL_P_H

#include <QString>

class KServiceUtilPrivate
{
public:
    /**
     * Lightweight implementation of QFileInfo::completeBaseName.
     *
     * Returns the complete base name of the file without the path.
     * The complete base name consists of all characters in the file up to (but not including) the last '.' character.
     *
     * Example: "/tmp/archive.tar.gz" --> "archive.tar"
     */
    static QString completeBaseName(const QString& filepath)
    {
        QString name = filepath;
        int pos = name.lastIndexOf(QLatin1Char('/'));
        if (pos != -1) {
            name.remove(0, pos + 1);
        }
        pos = name.lastIndexOf(QLatin1Char('.'));
        if (pos != -1) {
            name.truncate(pos);
        }
        return name;
    }
};

#endif
