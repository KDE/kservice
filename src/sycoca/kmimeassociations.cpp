/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2008 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kmimeassociations_p.h"
#include "sycocadebug.h"
#include <KConfig>
#include <KConfigGroup>
#include <QDebug>
#include <QFile>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <kservice.h>
#include <kservicefactory_p.h>

KMimeAssociations::KMimeAssociations(KOfferHash &offerHash, KServiceFactory *serviceFactory)
    : m_offerHash(offerHash)
    , m_serviceFactory(serviceFactory)
{
}

/*

The goal of this class is to parse mimeapps.list files, which are used to
let users configure the application-MIME type associations.

Example file:

[Added Associations]
text/plain=gnome-gedit.desktop;gnu-emacs.desktop;

[Removed Associations]
text/plain=gnome-gedit.desktop;gnu-emacs.desktop;

[Default Applications]
text/plain=kate.desktop;
*/

QStringList KMimeAssociations::mimeAppsFiles()
{
    QStringList mimeappsFileNames;
    // make the list of possible filenames from the spec ($desktop-mimeapps.list, then mimeapps.list)
    const QString desktops = QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP"));
    const auto list = desktops.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString &desktop : list) {
        mimeappsFileNames.append(desktop.toLower() + QLatin1String("-mimeapps.list"));
    }
    mimeappsFileNames.append(QStringLiteral("mimeapps.list"));
    const QStringList mimeappsDirs = mimeAppsDirs();
    QStringList mimeappsFiles;
    // collect existing files
    for (const QString &dir : mimeappsDirs) {
        for (const QString &file : std::as_const(mimeappsFileNames)) {
            const QString filePath = dir + QLatin1Char('/') + file;
            if (QFile::exists(filePath) && !mimeappsFiles.contains(filePath)) {
                mimeappsFiles.append(filePath);
            }
        }
    }
    return mimeappsFiles;
}

QStringList KMimeAssociations::mimeAppsDirs()
{
    // list the dirs in the order of the spec (XDG_CONFIG_HOME, XDG_CONFIG_DIRS, XDG_DATA_HOME, XDG_DATA_DIRS)
    return QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation) + QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
}

void KMimeAssociations::parseAllMimeAppsList()
{
    int basePreference = 1000; // start high :)
    const QStringList mimeappsFiles = KMimeAssociations::mimeAppsFiles();
    QListIterator<QString> mimeappsIter(mimeappsFiles);
    mimeappsIter.toBack();
    while (mimeappsIter.hasPrevious()) { // global first, then local.
        const QString mimeappsFile = mimeappsIter.previous();
        // qDebug() << "Parsing" << mimeappsFile;
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

    // Default Applications is preferred over Added Associations.
    // Other than that, they work the same...
    // add 25 to the basePreference to make sure those service offers will have higher preferences
    // 25 is arbitrary half of the allocated preference indices for the current parsed mimeapps.list file, defined line 86
    parseAddedAssociations(KConfigGroup(&profile, "Default Applications"), file, basePreference + 25);
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
            qCDebug(SYCOCA) << file << "specifies unknown MIME type" << mimeName << "in" << group.name();
        } else {
            int pref = basePreference;
            for (const QString &service : services) {
                KService::Ptr pService = m_serviceFactory->findServiceByStorageId(service);
                if (!pService) {
                    qCDebug(SYCOCA) << file << "specifies unknown service" << service << "in" << group.name();
                } else {
                    // qDebug() << "adding mime" << resolvedMimeName << "to service" << pService->entryPath() << "pref=" << pref;
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 69)
                    m_offerHash.addServiceOffer(resolvedMimeName, KServiceOffer(pService, pref, 0, pService->allowAsDefault()));
#else
                    m_offerHash.addServiceOffer(resolvedMimeName, KServiceOffer(pService, pref, 0));
#endif
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
            KService::Ptr pService = m_serviceFactory->findServiceByStorageId(service);
            if (!pService) {
                // qDebug() << file << "specifies unknown service" << service << "in" << group.name();
            } else {
                // qDebug() << "removing mime" << mime << "from service" << pService.data() << pService->entryPath();
                m_offerHash.removeServiceOffer(mime, pService);
            }
        }
    }
}

void KOfferHash::addServiceOffer(const QString &serviceType, const KServiceOffer &offer)
{
    KService::Ptr service = offer.service();
    // qDebug() << "Adding" << service->entryPath() << "to" << serviceType << offer.preference();
    ServiceTypeOffersData &data = m_serviceTypeData[serviceType]; // find or create
    QList<KServiceOffer> &offers = data.offers;
    QSet<KService::Ptr> &offerSet = data.offerSet;
    if (!offerSet.contains(service)) {
        offers.append(offer);
        offerSet.insert(service);
    } else {
        // qDebug() << service->entryPath() << "already in" << serviceType;
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
