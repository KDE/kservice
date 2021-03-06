/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "nsaplugin.h"
#include <KExportPlugin>
#include <QDebug>

NSAPlugin::NSAPlugin(QObject *parent, const QVariantList &args)
    : QObject(parent)
    , m_pluginInfo(args)
{
    // qDebug() << "SUCCESS!" << m_pluginInfo.pluginName() << m_pluginInfo.name() << m_pluginInfo.comment();
    setObjectName(m_pluginInfo.comment());
}

K_PLUGIN_FACTORY_WITH_JSON(nsaplugin, "fakeplugin.json", registerPlugin<NSAPlugin>();)

#include "nsaplugin.moc"
