/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Torben Weis <weis@kde.org>
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include <QtGlobal>
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

#include "kplugintrader.h"
#include "ktraderparsetree_p.h"

#include <KPluginLoader>
#include <KPluginMetaData>
#include <QVector>

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 82)

using namespace KTraderParse;

KPluginTrader *KPluginTrader::self()
{
    static KPluginTrader trader;
    return &trader;
}

KPluginTrader::KPluginTrader()
    : d(nullptr)
{
}

KPluginTrader::~KPluginTrader()
{
}

void KPluginTrader::applyConstraints(KPluginInfo::List &lst, const QString &constraint)
{
    if (lst.isEmpty() || constraint.isEmpty()) {
        return;
    }

    const ParseTreeBase::Ptr constr = parseConstraints(constraint); // for ownership
    const ParseTreeBase *pConstraintTree = constr.data(); // for speed

    if (!constr) { // parse error
        lst.clear();
    } else {
        // Find all plugin information matching the constraint and remove the rest
        KPluginInfo::List::iterator it = lst.begin();
        while (it != lst.end()) {
            if (matchConstraintPlugin(pConstraintTree, *it, lst) != 1) {
                it = lst.erase(it);
            } else {
                ++it;
            }
        }
    }
}

KPluginInfo::List KPluginTrader::query(const QString &subDirectory, const QString &servicetype, const QString &constraint)
{
    auto filter = [&](const KPluginMetaData &md) -> bool {
        const auto &types = md.serviceTypes();
        if (!types.isEmpty() && types.contains(servicetype)) {
            return true;
        }
        // handle compatibility JSON:
        const auto &data = md.rawData();
        const auto &jsonTypes = data.value(QStringLiteral("X-KDE-ServiceTypes")).toVariant().toStringList();
        if (!jsonTypes.isEmpty() && jsonTypes.contains(servicetype)) {
            return true;
        }
        return data.value(QStringLiteral("ServiceTypes")).toVariant().toStringList().contains(servicetype);
    };
    QVector<KPluginMetaData> plugins = servicetype.isEmpty() ? KPluginLoader::findPlugins(subDirectory) : KPluginLoader::findPlugins(subDirectory, filter);
    KPluginInfo::List lst = KPluginInfo::fromMetaData(plugins);
    applyConstraints(lst, constraint);
    return lst;
}

#endif
