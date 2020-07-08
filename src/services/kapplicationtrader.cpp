/* This file is part of the KDE libraries
   Copyright (C) 2000 Torben Weis <weis@kde.org>
   Copyright (C) 2006-2020 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kapplicationtrader.h"

#include "ksycoca.h"
#include "ksycoca_p.h"
#include "kservicetypefactory_p.h"
#include "kservicefactory_p.h"
#include "kmimetypefactory_p.h"
#include "servicesdebug.h"

#include <QMimeDatabase>

static KService::List mimeTypeSycocaServiceOffers(const QString &mimeType)
{
    KService::List lst;
    QMimeDatabase db;
    QString mime = db.mimeTypeForName(mimeType).name();
    if (mime.isEmpty()) {
        if (!mimeType.startsWith(QLatin1String("x-scheme-handler/"))) { // don't warn for unknown scheme handler mimetypes
            qCWarning(SERVICES) << "KApplicationTrader: mimeType" << mimeType << "not found";
            return lst; // empty
        }
        mime = mimeType;
    }
    KSycoca::self()->ensureCacheValid();
    KMimeTypeFactory *factory = KSycocaPrivate::self()->mimeTypeFactory();
    const int offset = factory->entryOffset(mime);
    if (!offset) {
        qCWarning(SERVICES) << "KApplicationTrader: mimeType" << mimeType << "not found";
        return lst; // empty
    }
    const int serviceOffersOffset = factory->serviceOffersOffset(mime);
    if (serviceOffersOffset > -1) {
        lst = KSycocaPrivate::self()->serviceFactory()->serviceOffers(offset, serviceOffersOffset);
    }
    return lst;
}

// Filter the offers for the requested mime type in order to keep only applications.
static void filterMimeTypeOffers(KService::List &list) // static, internal
{
    KServiceType::Ptr genericServiceTypePtr = KServiceType::serviceType(QStringLiteral("Application"));
    Q_ASSERT(genericServiceTypePtr);

    KSycoca::self()->ensureCacheValid();
    KServiceFactory *serviceFactory = KSycocaPrivate::self()->serviceFactory();

    // Remove non-Applications (TODO KF6: kill plugin desktop files, then kill this code)
    auto removeFunc = [&](const KService::Ptr &serv) {
        return !serviceFactory->hasOffer(genericServiceTypePtr, serv);
    };
    list.erase(std::remove_if(list.begin(), list.end(), removeFunc), list.end());
}

static void applyFilter(KService::List &list, KApplicationTrader::FilterFunc filterFunc)
{
    if (list.isEmpty()) {
        return;
    }

    // Find all services matching the constraint
    // and remove the other ones
    auto removeFunc = [&](const KService::Ptr &serv) {
        return !serv->showInCurrentDesktop() || (filterFunc && !filterFunc(serv));
    };
    list.erase(std::remove_if(list.begin(), list.end(), removeFunc), list.end());
}

KService::List KApplicationTrader::query(FilterFunc filterFunc)
{
    // Get all applications
    KSycoca::self()->ensureCacheValid();
    KServiceType::Ptr servTypePtr = KSycocaPrivate::self()->serviceTypeFactory()->findServiceTypeByName(QStringLiteral("Application"));
    Q_ASSERT(servTypePtr);
    if (servTypePtr->serviceOffersOffset() == -1) {
        return KService::List();
    }

    KService::List lst = KSycocaPrivate::self()->serviceFactory()->serviceOffers(servTypePtr);

    applyFilter(lst, filterFunc);

    qCDebug(SERVICES) << "query returning" << lst.count() << "offers";
    return lst;
}

KService::List KApplicationTrader::queryByMimeType(const QString &mimeType,
                                                   FilterFunc filterFunc)
{
    // Get all services of this mime type.
    KService::List lst = mimeTypeSycocaServiceOffers(mimeType);
    filterMimeTypeOffers(lst);

    applyFilter(lst, filterFunc);

    qCDebug(SERVICES) << "query for mimeType" << mimeType << "returning" << lst.count() << "offers";
    return lst;
}

KService::Ptr KApplicationTrader::preferredService(const QString &mimeType)
{
    const KService::List offers = queryByMimeType(mimeType);
    if (!offers.isEmpty()) {
        return offers.at(0);
    }
    return KService::Ptr();
}

bool KApplicationTrader::isSubsequence(const QString &pattern, const QString &text, Qt::CaseSensitivity cs)
{
    if (pattern.isEmpty()) {
        return false;
    }
    const bool chk_case = cs == Qt::CaseSensitive;

    QString::const_iterator i = text.constBegin(), j = pattern.constBegin();
    for (; i != text.constEnd() && j != pattern.constEnd(); ++i) {
        if ((chk_case && *i == *j) || (!chk_case && i->toLower() == j->toLower())) {
            ++j;
        }
    }
    return j == pattern.constEnd();
}


