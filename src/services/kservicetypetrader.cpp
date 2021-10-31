/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kservicetypetrader.h"

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 90)

#include "kservicefactory_p.h"
#include "kservicetype.h"
#include "kservicetypefactory_p.h"
#include "ksycoca.h"
#include "ksycoca_p.h"
#include "ktraderparsetree_p.h"
#include <kservicetypeprofile.h>

#include "servicesdebug.h"

using namespace KTraderParse;

// --------------------------------------------------

namespace KServiceTypeProfile
{
KServiceOfferList sortServiceTypeOffers(const KServiceOfferList &list, const QString &servicetype);
}

class KServiceTypeTraderSingleton
{
public:
    KServiceTypeTrader instance;
};

Q_GLOBAL_STATIC(KServiceTypeTraderSingleton, s_globalServiceTypeTrader)

KServiceTypeTrader *KServiceTypeTrader::self()
{
    return &s_globalServiceTypeTrader()->instance;
}

KServiceTypeTrader::KServiceTypeTrader()
    : d(nullptr)
{
}

KServiceTypeTrader::~KServiceTypeTrader()
{
}

// shared with KMimeTypeTrader
void KServiceTypeTrader::applyConstraints(KService::List &lst, const QString &constraint)
{
    if (lst.isEmpty() || constraint.isEmpty()) {
        return;
    }

    const ParseTreeBase::Ptr constr = parseConstraints(constraint); // for ownership
    const ParseTreeBase *pConstraintTree = constr.data(); // for speed

    if (!constr) { // parse error
        lst.clear();
    } else {
        // Find all services matching the constraint
        // and remove the other ones
        auto isMatch = [=](const KService::Ptr &service) {
            return matchConstraint(pConstraintTree, service, lst) != 1;
        };

        lst.erase(std::remove_if(lst.begin(), lst.end(), isMatch), lst.end());
    }
}

KServiceOfferList KServiceTypeTrader::weightedOffers(const QString &serviceType) // static, internal
{
    // qDebug() << "KServiceTypeTrader::weightedOffers( " << serviceType << " )";

    KSycoca::self()->ensureCacheValid();
    KServiceType::Ptr servTypePtr = KSycocaPrivate::self()->serviceTypeFactory()->findServiceTypeByName(serviceType);
    if (!servTypePtr) {
        qCWarning(SERVICES) << "KServiceTypeTrader: serviceType" << serviceType << "not found";
        return KServiceOfferList();
    }
    if (servTypePtr->serviceOffersOffset() == -1) { // no offers in ksycoca
        return KServiceOfferList();
    }

    // First, get all offers known to ksycoca.
    const KServiceOfferList services = KSycocaPrivate::self()->serviceFactory()->offers(servTypePtr->offset(), servTypePtr->serviceOffersOffset());

    const KServiceOfferList offers = KServiceTypeProfile::sortServiceTypeOffers(services, serviceType);

    return offers;
}

KService::List KServiceTypeTrader::defaultOffers(const QString &serviceType, const QString &constraint) const
{
    KSycoca::self()->ensureCacheValid();
    KServiceType::Ptr servTypePtr = KSycocaPrivate::self()->serviceTypeFactory()->findServiceTypeByName(serviceType);
    if (!servTypePtr) {
        qCWarning(SERVICES) << "KServiceTypeTrader: serviceType" << serviceType << "not found";
        return KService::List();
    }
    if (servTypePtr->serviceOffersOffset() == -1) {
        return KService::List();
    }

    KService::List lst = KSycocaPrivate::self()->serviceFactory()->serviceOffers(servTypePtr->offset(), servTypePtr->serviceOffersOffset());

    applyConstraints(lst, constraint);

    // qDebug() << "query for serviceType " << serviceType << constraint
    //             << " : returning " << lst.count() << " offers" << endl;
    return lst;
}

KService::List KServiceTypeTrader::query(const QString &serviceType, const QString &constraint) const
{
    if (!KServiceTypeProfile::hasProfile(serviceType)) {
        // Fast path: skip the profile stuff if there's none (to avoid kservice->serviceoffer->kservice)
        // The ordering according to initial preferences is done by kbuildsycoca
        return defaultOffers(serviceType, constraint);
    }

    // Get all services of this service type.
    const KServiceOfferList offers = weightedOffers(serviceType);
    KService::List lst;
    lst.reserve(offers.size());

    // Now extract only the services; the weighting was only used for sorting.
    std::transform(offers.cbegin(), offers.cend(), std::back_inserter(lst), [](const KServiceOffer &offer) {
        return offer.service();
    });

    applyConstraints(lst, constraint);

    // qDebug() << "query for serviceType " << serviceType << constraint
    //             << " : returning " << lst.count() << " offers" << endl;
    return lst;
}

KService::Ptr KServiceTypeTrader::preferredService(const QString &serviceType) const
{
    const KServiceOfferList offers = weightedOffers(serviceType);

    KServiceOfferList::const_iterator itOff = offers.begin();
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 67)
    // Look for the first one that is allowed as default.
    // Since the allowed-as-default are first anyway, we only have
    // to look at the first one to know.
    if (itOff != offers.end() && (*itOff).allowAsDefault()) {
#else
    if (itOff != offers.end()) {
#endif
        return (*itOff).service();
    }

    // qDebug() << "No offers, or none allowed as default";
    return KService::Ptr();
}

#endif
