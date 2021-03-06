/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef NSAPLUGIN_H
#define NSAPLUGIN_H

#include <QObject>

#include <KPluginFactory>
#include <kplugininfo.h>

class NSAPlugin : public QObject
{
    Q_OBJECT

public:
    explicit NSAPlugin(QObject *parent, const QVariantList &args);

private:
    KPluginInfo m_pluginInfo;
};
// Q_DECLARE_METATYPE(NSAPlugin*)

#endif // NSAPLUGIN_H
