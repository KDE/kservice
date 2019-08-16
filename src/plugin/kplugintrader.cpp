/* This file is part of the KDE libraries
   Copyright (C) 2000 Torben Weis <weis@kde.org>
   Copyright (C) 2006 David Faure <faure@kde.org>
   Copyright 2013 Sebastian Kügler <sebas@kde.org>

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

#include "kplugintrader.h"
#include "ktraderparsetree_p.h"


#include <KPluginLoader>
#include <KPluginMetaData>

using namespace KTraderParse;

class KPluginTraderSingleton
{
public:
    KPluginTrader instance;
};

Q_GLOBAL_STATIC(KPluginTraderSingleton, s_globalPluginTrader)

KPluginTrader *KPluginTrader::self()
{
    return &s_globalPluginTrader()->instance;
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
    auto filter = [&](const KPluginMetaData &md) -> bool
    {
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
    QVector<KPluginMetaData> plugins = servicetype.isEmpty() ?
            KPluginLoader::findPlugins(subDirectory) : KPluginLoader::findPlugins(subDirectory, filter);
    KPluginInfo::List lst = KPluginInfo::fromMetaData(plugins);
    applyConstraints(lst, constraint);
    return lst;
}
