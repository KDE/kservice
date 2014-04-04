/* This file is part of the KDE libraries
   Copyright (C) 2000 Torben Weis <weis@kde.org>
   Copyright (C) 2006 David Faure <faure@kde.org>

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

#include "kmimetypetrader.h"

#include "kservicetypeprofile.h"
#include "kservicetype.h"
#include "kservicetypetrader.h"
#include "kservicefactory.h"
#include "kmimetypefactory_p.h"
#include <kconfiggroup.h>
#include <qmimedatabase.h>
#include <QDebug>
#include <QFile>

class KMimeTypeTrader::Private
{
public:
    Private() {}
};

class KMimeTypeTraderSingleton
{
public:
    KMimeTypeTrader instance;
};

Q_GLOBAL_STATIC(KMimeTypeTraderSingleton, s_self)

KMimeTypeTrader *KMimeTypeTrader::self()
{
    return &s_self()->instance;
}

KMimeTypeTrader::KMimeTypeTrader()
    : d(new Private())
{
}

KMimeTypeTrader::~KMimeTypeTrader()
{
    delete d;
}

static KServiceOfferList mimeTypeSycocaOffers(const QString &mimeType)
{
    KServiceOfferList lst;

    QMimeDatabase db;
    QString mime = db.mimeTypeForName(mimeType).name();
    if (mime.isEmpty()) {
        if (!mimeType.startsWith(QLatin1String("x-scheme-handler/"))) { // don't warn for unknown scheme handler mimetypes
            qWarning() << "KMimeTypeTrader: mimeType" << mimeType << "not found";
            return lst; // empty
        }
        mime = mimeType;
    }
    KMimeTypeFactory *factory = KMimeTypeFactory::self();
    const int offset = factory->entryOffset(mime);
    if (!offset) { // shouldn't happen, now that we know the mimetype exists
        if (!mimeType.startsWith(QLatin1String("x-scheme-handler/"))) { // don't warn for unknown scheme handler mimetypes
            qDebug() << "KMimeTypeTrader: no entry offset for" << mimeType;
        }
        return lst; // empty
    }

    const int serviceOffersOffset = factory->serviceOffersOffset(mime);
    if (serviceOffersOffset > -1) {
        lst = KServiceFactory::self()->offers(offset, serviceOffersOffset);
    }
    return lst;
}

static KService::List mimeTypeSycocaServiceOffers(const QString &mimeType)
{
    KService::List lst;
    QMimeDatabase db;
    const QString mime = db.mimeTypeForName(mimeType).name();
    if (mime.isEmpty()) {
        qWarning() << "KMimeTypeTrader: mimeType" << mimeType << "not found";
        return lst; // empty
    }
    KMimeTypeFactory *factory = KMimeTypeFactory::self();
    const int offset = factory->entryOffset(mime);
    if (!offset) {
        qWarning() << "KMimeTypeTrader: mimeType" << mimeType << "not found";
        return lst; // empty
    }
    const int serviceOffersOffset = factory->serviceOffersOffset(mime);
    if (serviceOffersOffset > -1) {
        lst = KServiceFactory::self()->serviceOffers(offset, serviceOffersOffset);
    }
    return lst;
}

#define CHECK_SERVICETYPE(genericServiceTypePtr) \
    if (!genericServiceTypePtr) { \
        qWarning() << "KMimeTypeTrader: couldn't find service type" << genericServiceType << \
                   "\nPlease ensure that the .desktop file for it is installed; then run kbuildsycoca5."; \
        return; \
    }

/**
 * Filter the offers for the requested mime type for the genericServiceType.
 *
 * @param list list of offers (key=service, value=initialPreference)
 * @param genericServiceType the generic service type (e.g. "Application" or "KParts/ReadOnlyPart")
 */
void KMimeTypeTrader::filterMimeTypeOffers(KServiceOfferList &list, const QString &genericServiceType) // static, internal
{
    KServiceType::Ptr genericServiceTypePtr = KServiceType::serviceType(genericServiceType);
    CHECK_SERVICETYPE(genericServiceTypePtr);

    QMutableListIterator<KServiceOffer> it(list);
    while (it.hasNext()) {
        const KService::Ptr servPtr = it.next().service();
        // Expand servPtr->hasServiceType( genericServiceTypePtr ) to avoid lookup each time:
        if (!KServiceFactory::self()->hasOffer(genericServiceTypePtr->offset(),
                                               genericServiceTypePtr->serviceOffersOffset(),
                                               servPtr->offset())
                || !servPtr->showInCurrentDesktop()) {
            it.remove();
        }
    }
}

void KMimeTypeTrader::filterMimeTypeOffers(KService::List &list, const QString &genericServiceType) // static, internal
{
    KServiceType::Ptr genericServiceTypePtr = KServiceType::serviceType(genericServiceType);
    CHECK_SERVICETYPE(genericServiceTypePtr);

    QMutableListIterator<KService::Ptr> it(list);
    while (it.hasNext()) {
        const KService::Ptr servPtr = it.next();
        // Expand servPtr->hasServiceType( genericServiceTypePtr ) to avoid lookup each time:
        if (!KServiceFactory::self()->hasOffer(genericServiceTypePtr->offset(),
                                               genericServiceTypePtr->serviceOffersOffset(),
                                               servPtr->offset())
                || !servPtr->showInCurrentDesktop()) {
            it.remove();
        }
    }
}

#undef CHECK_SERVICETYPE

KService::List KMimeTypeTrader::query(const QString &mimeType,
                                      const QString &genericServiceType,
                                      const QString &constraint) const
{
    // Get all services of this mime type.
    KService::List lst = mimeTypeSycocaServiceOffers(mimeType);
    filterMimeTypeOffers(lst, genericServiceType);

    KServiceTypeTrader::applyConstraints(lst, constraint);

    //qDebug() << "query for mimeType " << mimeType << ", " << genericServiceType
    //         << " : returning " << lst.count() << " offers";
    return lst;
}

KService::Ptr KMimeTypeTrader::preferredServiceImpl(const QString &mimeType, const QString &genericServiceType)
{
    // First, get all offers known to ksycoca.
    KServiceOfferList offers = mimeTypeSycocaOffers(mimeType);

    // Assign preferences from the profile to those offers - and filter for genericServiceType
    Q_ASSERT(!genericServiceType.isEmpty());
    filterMimeTypeOffers(offers, genericServiceType);

    KServiceOfferList::const_iterator itOff = offers.constBegin();
    // Look for the first one that is allowed as default.
    // Since the allowed-as-default are first anyway, we only have
    // to look at the first one to know.
    if (itOff != offers.constEnd() && (*itOff).allowAsDefault()) {
        return (*itOff).service();
    }

    //qDebug() << "No offers, or none allowed as default";
    return KService::Ptr();
}

KService::Ptr KMimeTypeTrader::preferredService(const QString &mimeType, const QString &genericServiceType)
{
    if (genericServiceType == QLatin1String("Application")) {
        qWarning("KMimeTypeTrader::preferredService(mimeType, \"Application\") is deprecated, please use KMimeTypeTrader::preferredApplication(mimeType)");
        return preferredApplication(mimeType);
    }
    return preferredServiceImpl(mimeType, genericServiceType);
}

// Return the various filenames that should be tried, e.g. kde-mimeapps.list and mimeapps.list
// http://standards.freedesktop.org/mime-apps-spec/mime-apps-spec-1.0.html
static QStringList mimeAppsListFileNames()
{
    static QStringList xdgCurrentDesktops = QString::fromLatin1(qgetenv("XDG_CURRENT_DESKTOP")).split(':');
    QStringList fileNames;
    foreach (const QString &desktop, xdgCurrentDesktops) {
        fileNames.append(desktop.toLower() + QStringLiteral("-mimeapps.list"));
    }
    fileNames.append(QStringLiteral("mimeapps.list"));
    return fileNames;
}

// Return the mimeapps.list files that actually exist in the system
// http://standards.freedesktop.org/mime-apps-spec/mime-apps-spec-1.0.html
static QStringList allMimeAppsList()
{
    QStringList files;
    static const auto addExistingFiles = [&files](const QStringList &dirs) {
        static QStringList fileNames = mimeAppsListFileNames();
        foreach (const QString &dir, dirs) {
            foreach (const QString &fileName, fileNames) {
                const QString file = dir + '/' + fileName;
                if (QFile::exists(file)) {
                    files.append(file);
                }
            }
        }
    };
    const QStringList configDirs = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    addExistingFiles(configDirs);
    const QStringList dataDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    addExistingFiles(dataDirs);
    return files;
}

KService::Ptr KMimeTypeTrader::preferredApplication(const QString &mimeType, PreferredApplicationBehavior behavior)
{
    // Iterate from most-local to most-global and stop at the first installed application mentionned
    // in the file.
    foreach(const QString &mimeAppsFile, allMimeAppsList()) {
        KConfig config(mimeAppsFile, KConfig::SimpleConfig);
        KConfigGroup group(&config, "Default Applications");
        foreach(const QString &desktopFile, group.readXdgListEntry(mimeType)) {
            KService::Ptr serv = KService::serviceByMenuId(desktopFile);
            if (serv) {
                return serv;
            }
        }
    }

    if (behavior == WithFallback) {
        return preferredServiceImpl(mimeType, "Application");
    }

    return KService::Ptr();
}
