/*
 *  Copyright (C) 2002-2006 David Faure   <faure@kde.org>
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
 */

#include <QCoreApplication>
#include <QDebug>
#include <kservice.h>
#include <kservicegroup.h>

int main(int argc, char *argv[])
{
    QCoreApplication k(argc, argv);

    KServiceGroup::Ptr root = KServiceGroup::root();
    KServiceGroup::List list = root->entries();

    KServiceGroup::Ptr first;

    qDebug("Found %d entries", list.count());
    for (KServiceGroup::List::ConstIterator it = list.constBegin();
            it != list.constEnd(); ++it) {
        KSycocaEntry::Ptr p = (*it);
        if (p->isType(KST_KService)) {
            KService::Ptr service(static_cast<KService*>(p.data()));
            qDebug("%s", qPrintable(service->name()));
            qDebug("%s", qPrintable(service->entryPath()));
        } else if (p->isType(KST_KServiceGroup)) {
            KServiceGroup::Ptr serviceGroup(static_cast<KServiceGroup*>(p.data()));
            qDebug("             %s -->", qPrintable(serviceGroup->caption()));
            if (!first) {
                first = serviceGroup;
            }
        } else {
            qDebug("KServiceGroup: Unexpected object in list!");
        }
    }

    if (first) {
        list = first->entries();
        qDebug("Found %d entries", list.count());
        for (KServiceGroup::List::ConstIterator it = list.constBegin();
                it != list.constEnd(); ++it) {
            KSycocaEntry::Ptr p = (*it);
            if (p->isType(KST_KService)) {
                KService::Ptr service(static_cast<KService*>(p.data()));
                qDebug("             %s", qPrintable(service->name()));
            } else if (p->isType(KST_KServiceGroup)) {
                KServiceGroup::Ptr serviceGroup(static_cast<KServiceGroup*>(p.data()));
                qDebug("             %s -->", qPrintable(serviceGroup->caption()));
            } else {
                qDebug("KServiceGroup: Unexpected object in list!");
            }
        }
    }

    return 0;
}
