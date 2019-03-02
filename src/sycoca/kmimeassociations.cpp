/*  This file is part of the KDE libraries
 *  Copyright 2008  David Faure  <faure@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License or ( at
 *  your option ) version 3 or, at the discretion of KDE e.V. ( which shall
 *  act as a proxy as in section 14 of the GPLv3 ), any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kmimeassociations_p.h"
#include <kservice.h>
#include <kservicefactory_p.h>
#include <kconfiggroup.h>
#include <kconfig.h>
#include <QDebug>
#include <QFile>
#include <qstandardpaths.h>
#include <qmimedatabase.h>
#include "sycocadebug.h"

KMimeAssociations::KMimeAssociations(KOfferHash &offerHash, KServiceFactory *serviceFactory)
    : m_offerHash(offerHash), m_serviceFactory(serviceFactory)
{
}

/*

The goal of this class is to parse mimeapps.list files, which are used to
let users configure the application-mimetype associations.

Example file:

[Added Associations]
text/plain=kate.desktop;

[Removed Associations]
text/plain=gnome-gedit.desktop;gnu-emacs.desktop;

*/

void KMimeAssociations::parseAllMimeAppsList()
{
    QStringList mimeappsFileNames;
    // make the list of possible filenames from the spec ($desktop-mimeapps.list, then mimeapps.list)
    const QString desktops = QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP"));
    const auto list = desktops.split(QLatin1Char(':'), QString::SkipEmptyParts);
    for (const QString &desktop : list) {
        mimeappsFileNames.append(desktop.toLower() + QLatin1String("-mimeapps.list"));
    }
    mimeappsFileNames.append(QStringLiteral("mimeapps.list"));
    // list the dirs in the order of the spec (XDG_CONFIG_HOME, XDG_CONFIG_DIRS, XDG_DATA_HOME, XDG_DATA_DIRS)
    const QStringList mimeappsDirs = QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation)
                                    + QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    QStringList mimeappsFiles;
    // collect existing files
    for (const QString &dir : mimeappsDirs) {
        for (const QString &file : qAsConst(mimeappsFileNames)) {
            const QString filePath = dir + QLatin1Char('/') + file;
            if (QFile::exists(filePath)) {
                mimeappsFiles.append(filePath);
            }
        }
    }
    //qDebug() << "FILE LIST:" << mimeappsFiles;

    int basePreference = 1000; // start high :)
    QListIterator<QString> mimeappsIter(mimeappsFiles);
    mimeappsIter.toBack();
    while (mimeappsIter.hasPrevious()) { // global first, then local.
        const QString mimeappsFile = mimeappsIter.previous();
        //qDebug() << "Parsing" << mimeappsFile;
        parseMimeAppsList(mimeappsFile, basePreference);
        basePreference += 50;
    }
}

void KMimeAssociations::parseMimeAppsList(const QString &file, int basePreference)
{
    KConfig profile(file, KConfig::SimpleConfig);
    if (file.endsWith(QLatin1String("/mimeapps.list"))) { // not for $desktop-mimeapps.list
        parseAddedAssociations(KConfigGroup(&profile, "Added Associations"), file, basePreference);
        parseRemovedAssociations(KConfigGroup(&profile, "Removed Associations"), file);

        // KDE extension for parts and plugins, see settings/filetypes/mimetypedata.cpp
        parseAddedAssociations(KConfigGroup(&profile, "Added KDE Service Associations"), file, basePreference);
        parseRemovedAssociations(KConfigGroup(&profile, "Removed KDE Service Associations"), file);
    }

    // TODO "Default Applications" is a separate query and a separate algorithm, says the spec.
    // For now this is better than nothing though.
    parseAddedAssociations(KConfigGroup(&profile, "Default Applications"), file, basePreference);
}

void KMimeAssociations::parseAddedAssociations(const KConfigGroup &group, const QString &file, int basePreference)
{
    Q_UNUSED(file) // except in debug statements
    QMimeDatabase db;
    const auto keyList = group.keyList();
    for (const QString &mimeName : keyList) {
        const QStringList services = group.readXdgListEntry(mimeName);
        const QString resolvedMimeName = mimeName.startsWith(QLatin1String("x-scheme-handler/")) ? mimeName : db.mimeTypeForName(mimeName).name();
        if (resolvedMimeName.isEmpty()) {
            qCDebug(SYCOCA) << file << "specifies unknown mimeType" << mimeName << "in" << group.name();
        } else {
            int pref = basePreference;
            for (const QString &service : services) {
                KService::Ptr pService = m_serviceFactory->findServiceByStorageId(service);
                if (!pService) {
                    qCDebug(SYCOCA) << file << "specifies unknown service" << service << "in" << group.name();
                } else {
                    //qDebug() << "adding mime" << resolvedMimeName << "to service" << pService->entryPath() << "pref=" << pref;
                    m_offerHash.addServiceOffer(resolvedMimeName, KServiceOffer(pService, pref, 0, pService->allowAsDefault()));
                    --pref;
                }
            }
        }
    }
}

void KMimeAssociations::parseRemovedAssociations(const KConfigGroup &group, const QString &file)
{
    Q_UNUSED(file) // except in debug statements
    const auto keyList = group.keyList();
    for (const QString &mime : keyList) {
        const QStringList services = group.readXdgListEntry(mime);
        for (const QString &service : services) {
            KService::Ptr pService =  m_serviceFactory->findServiceByStorageId(service);
            if (!pService) {
                //qDebug() << file << "specifies unknown service" << service << "in" << group.name();
            } else {
                //qDebug() << "removing mime" << mime << "from service" << pService.data() << pService->entryPath();
                m_offerHash.removeServiceOffer(mime, pService);
            }
        }
    }
}

void KOfferHash::addServiceOffer(const QString &serviceType, const KServiceOffer &offer)
{
    KService::Ptr service = offer.service();
    //qDebug() << "Adding" << service->entryPath() << "to" << serviceType << offer.preference();
    ServiceTypeOffersData &data = m_serviceTypeData[serviceType]; // find or create
    QList<KServiceOffer> &offers = data.offers;
    QSet<KService::Ptr> &offerSet = data.offerSet;
    if (!offerSet.contains(service)) {
        offers.append(offer);
        offerSet.insert(service);
    } else {
        //qDebug() << service->entryPath() << "already in" << serviceType;
        // This happens when mimeapps.list mentions a service (to make it preferred)
        // Update initialPreference to qMax(existing offer, new offer)
        QMutableListIterator<KServiceOffer> sfit(data.offers);
        while (sfit.hasNext()) {
            if (sfit.next().service() == service) { // we can compare KService::Ptrs because they are from the memory hash
                sfit.value().setPreference(qMax(sfit.value().preference(), offer.preference()));
            }
        }
    }
}

void KOfferHash::removeServiceOffer(const QString &serviceType, const KService::Ptr &service)
{
    ServiceTypeOffersData &data = m_serviceTypeData[serviceType]; // find or create
    data.removedOffers.insert(service);
    data.offerSet.remove(service);
    QMutableListIterator<KServiceOffer> sfit(data.offers);
    while (sfit.hasNext()) {
        if (sfit.next().service()->storageId() == service->storageId()) {
            sfit.remove();
        }
    }
}

bool KOfferHash::hasRemovedOffer(const QString &serviceType, const KService::Ptr &service) const
{
    QHash<QString, ServiceTypeOffersData>::const_iterator it = m_serviceTypeData.find(serviceType);
    if (it != m_serviceTypeData.end()) {
        return (*it).removedOffers.contains(service);
    }
    return false;
}
