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

#include "ksycoca.h"
#include "ksycoca_p.h"
#include "kservicetypeprofile.h"
#include "kservicetype.h"
#include "kservicetypetrader.h"
#include "kservicefactory_p.h"
#include "kmimetypefactory_p.h"
#include "servicesdebug.h"
#include <qmimedatabase.h>

class KMimeTypeTraderPrivate
{
public:
    KMimeTypeTraderPrivate() {}
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
    : d(new KMimeTypeTraderPrivate())
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
            qCWarning(SERVICES) << "KMimeTypeTrader: mimeType" << mimeType << "not found";
            return lst; // empty
        }
        mime = mimeType;
    }
    KSycoca::self()->ensureCacheValid();
    KMimeTypeFactory *factory = KSycocaPrivate::self()->mimeTypeFactory();
    const int offset = factory->entryOffset(mime);
    if (!offset) { // shouldn't happen, now that we know the mimetype exists
        if (!mimeType.startsWith(QLatin1String("x-scheme-handler/"))) { // don't warn for unknown scheme handler mimetypes
            qCDebug(SERVICES) << "KMimeTypeTrader: no entry offset for" << mimeType;
        }
        return lst; // empty
    }

    const int serviceOffersOffset = factory->serviceOffersOffset(mime);
    if (serviceOffersOffset > -1) {
        lst = KSycocaPrivate::self()->serviceFactory()->offers(offset, serviceOffersOffset);
    }
    return lst;
}

static KService::List mimeTypeSycocaServiceOffers(const QString &mimeType)
{
    KService::List lst;
    QMimeDatabase db;
    QString mime = db.mimeTypeForName(mimeType).name();
    if (mime.isEmpty()) {
        if (!mimeType.startsWith(QLatin1String("x-scheme-handler/"))) { // don't warn for unknown scheme handler mimetypes
            qCWarning(SERVICES) << "KMimeTypeTrader: mimeType" << mimeType << "not found";
            return lst; // empty
        }
        mime = mimeType;
    }
    KSycoca::self()->ensureCacheValid();
    KMimeTypeFactory *factory = KSycocaPrivate::self()->mimeTypeFactory();
    const int offset = factory->entryOffset(mime);
    if (!offset) {
        qCWarning(SERVICES) << "KMimeTypeTrader: mimeType" << mimeType << "not found";
        return lst; // empty
    }
    const int serviceOffersOffset = factory->serviceOffersOffset(mime);
    if (serviceOffersOffset > -1) {
        lst = KSycocaPrivate::self()->serviceFactory()->serviceOffers(offset, serviceOffersOffset);
    }
    return lst;
}

#define CHECK_SERVICETYPE(genericServiceTypePtr) \
    if (!genericServiceTypePtr) { \
        qCWarning(SERVICES) << "KMimeTypeTrader: couldn't find service type" << genericServiceType << \
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

    KSycoca::self()->ensureCacheValid();

    QMutableListIterator<KServiceOffer> it(list);
    while (it.hasNext()) {
        const KService::Ptr servPtr = it.next().service();
        // Expand servPtr->hasServiceType( genericServiceTypePtr ) to avoid lookup each time:
        if (!KSycocaPrivate::self()->serviceFactory()->hasOffer(genericServiceTypePtr->offset(),
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

    KSycoca::self()->ensureCacheValid();

    QMutableListIterator<KService::Ptr> it(list);
    while (it.hasNext()) {
        const KService::Ptr servPtr = it.next();
        // Expand servPtr->hasServiceType( genericServiceTypePtr ) to avoid lookup each time:
        if (!KSycocaPrivate::self()->serviceFactory()->hasOffer(genericServiceTypePtr->offset(),
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

    //qCDebug(SERVICES) << "query for mimeType " << mimeType << ", " << genericServiceType
    //         << " : returning " << lst.count() << " offers";
    return lst;
}

KService::Ptr KMimeTypeTrader::preferredService(const QString &mimeType, const QString &genericServiceType)
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

    //qCDebug(SERVICES) << "No offers, or none allowed as default";
    return KService::Ptr();
}
