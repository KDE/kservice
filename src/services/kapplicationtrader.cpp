/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 2006-2020 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kapplicationtrader.h"

#include "kmimetypefactory_p.h"
#include "kservicefactory_p.h"
#include "ksycoca.h"
#include "ksycoca_p.h"
#include "servicesdebug.h"

#include <QMimeDatabase>

#include <KConfigGroup>
#include <KDesktopFile>
#include <KSharedConfig>

using namespace Qt::Literals;

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
        if (!mimeType.startsWith(QLatin1String("x-scheme-handler/"))) { // don't warn for unknown scheme handler mimetypes
            qCWarning(SERVICES) << "KApplicationTrader: mimeType" << mimeType << "not found";
        }
        return lst; // empty
    }
    const int serviceOffersOffset = factory->serviceOffersOffset(mime);
    if (serviceOffersOffset > -1) {
        lst = KSycocaPrivate::self()->serviceFactory()->serviceOffers(offset, serviceOffersOffset);
    }
    return lst;
}

static void applyFilter(KService::List &list, KApplicationTrader::FilterFunc filterFunc, bool mustShowInCurrentDesktop)
{
    if (list.isEmpty()) {
        return;
    }

    // Find all services matching the constraint
    // and remove the other ones
    auto removeFunc = [&](const KService::Ptr &serv) {
        return (filterFunc && !filterFunc(serv)) || (mustShowInCurrentDesktop && !serv->showInCurrentDesktop());
    };
    list.erase(std::remove_if(list.begin(), list.end(), removeFunc), list.end());
}

KService::List KApplicationTrader::query(FilterFunc filterFunc)
{
    // Get all applications
    KSycoca::self()->ensureCacheValid();
    KService::List lst = KSycocaPrivate::self()->serviceFactory()->allServices();

    applyFilter(lst, filterFunc, true); // true = filter out service with NotShowIn=KDE or equivalent

    qCDebug(SERVICES) << "query returning" << lst.count() << "offers";
    return lst;
}

KService::List KApplicationTrader::queryByMimeType(const QString &mimeType, FilterFunc filterFunc)
{
    // Get all services of this MIME type.
    KService::List lst = mimeTypeSycocaServiceOffers(mimeType);

    applyFilter(lst, filterFunc, false); // false = allow NotShowIn=KDE services listed in mimeapps.list

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

void KApplicationTrader::setPreferredService(const QString &mimeType, const KService::Ptr service)
{
    if (mimeType.isEmpty() || !(service && service->isValid())) {
        return;
    }
    KSharedConfig::Ptr profile = KSharedConfig::openConfig(QStringLiteral("mimeapps.list"), KConfig::NoGlobals, QStandardPaths::GenericConfigLocation);

    // Save the default application according to mime-apps-spec 1.0
    KConfigGroup defaultApp(profile, QStringLiteral("Default Applications"));
    defaultApp.writeXdgListEntry(mimeType, QStringList(service->storageId()));

    KConfigGroup addedApps(profile, QStringLiteral("Added Associations"));
    QStringList apps = addedApps.readXdgListEntry(mimeType);
    apps.removeAll(service->storageId());
    apps.prepend(service->storageId()); // make it the preferred app
    addedApps.writeXdgListEntry(mimeType, apps);

    profile->sync();

    // Also make sure the "auto embed" setting for this MIME type is off
    KSharedConfig::Ptr fileTypesConfig = KSharedConfig::openConfig(QStringLiteral("filetypesrc"), KConfig::NoGlobals);
    fileTypesConfig->group(QStringLiteral("EmbedSettings")).writeEntry(QStringLiteral("embed-") + mimeType, false);
    fileTypesConfig->sync();
}

bool KApplicationTrader::isSubsequence(const QString &pattern, const QString &text, Qt::CaseSensitivity cs)
{
    if (pattern.isEmpty()) {
        return false;
    }
    const bool chk_case = cs == Qt::CaseSensitive;

    auto textIt = text.cbegin();
    auto patternIt = pattern.cbegin();
    for (; textIt != text.cend() && patternIt != pattern.cend(); ++textIt) {
        if ((chk_case && *textIt == *patternIt) || (!chk_case && textIt->toLower() == patternIt->toLower())) {
            ++patternIt;
        }
    }
    return patternIt == pattern.cend();
}

KService::List KApplicationTrader::queryByIntent(const QString &intent, const QString &scope)
{
    KService::List result;

    const auto intentCacheFiles = QStandardPaths::locateAll(QStandardPaths::ApplicationsLocation, u"intent.cache"_s);
    for (const auto &intentCacheFile : intentCacheFiles) {
        QStringList services;
        KDesktopFile cache(intentCacheFile);
        if (scope.isEmpty()) {
            const auto grp = cache.group(u"Intent Cache"_s);
            services = grp.readXdgListEntry(intent);
        } else {
            const auto grp = cache.group(intent);
            services = grp.readXdgListEntry(scope);
        }

        for (const auto &serviceName : services) {
            if (std::ranges::any_of(result, [serviceName](const auto &s) {
                    return s->desktopEntryName() == serviceName;
                })) {
                continue;
            }
            auto s = KService::serviceByDesktopName(serviceName);
            if (s) {
                result.push_back(std::move(s));
            }
        }
    }

    return result;
}

KService::Ptr KApplicationTrader::preferredServiceForIntent(const QString &intent, const QString &scope)
{
    const QString desktops = QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP")).toLower();
    const auto list = QStringView(desktops).split(':'_L1, Qt::SkipEmptyParts);
    QStringList fileNames;
    fileNames.reserve(list.size() + 1);
    for (const auto &desktop : list) {
        fileNames.push_back(desktop + "-intentapps.list"_L1);
    }
    fileNames.push_back(u"intentapp.list"_s);

    const auto dirs =
        QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation) + QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const auto &dir : dirs) {
        for (const auto &fileName : fileNames) {
            KDesktopFile f(dir + '/'_L1 + fileName);
            QStringList services;
            if (scope.isEmpty()) {
                const auto grp = f.group(u"Default Applications"_s);
                services = grp.readXdgListEntry(intent);
            } else {
                const auto grp = f.group(intent);
                services = grp.readXdgListEntry(scope);
            }

            for (const auto &serviceName : services) {
                auto s = KService::serviceByDesktopName(serviceName);
                if (s) {
                    if (!s->supportedIntents().contains(intent)) {
                        continue;
                    }
                    if (!scope.isEmpty() && !s->supportedScopesForIntent(intent).contains(scope)) {
                        continue;
                    }
                    return s;
                }
            }
        }
    }

    // no preferred service defined, pick any
    const auto l = KApplicationTrader::queryByIntent(intent, scope);
    return l.isEmpty() ? KService::Ptr() : l.front();
}

void KApplicationTrader::setPreferredServiceForIntent(const QString &intent, const KService::Ptr &service, const QString &scope)
{
    if (!service) {
        return;
    }

    KDesktopFile f(QStandardPaths::GenericConfigLocation, u"intentapp.list"_s);
    if (scope.isEmpty()) {
        auto grp = f.group(u"Default Applications"_s);
        grp.writeEntry(intent, service->desktopEntryName());
    } else {
        auto grp = f.group(intent);
        grp.writeEntry(scope, service->desktopEntryName());
    }
}
