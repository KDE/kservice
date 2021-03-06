/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KAUTOSTARTTEST_H
#define KAUTOSTARTTEST_H

#include <QObject>

class KAutostartTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testStartDetection_data();
    void testStartDetection();
    void testStartInEnvDetection_data();
    void testStartInEnvDetection();
    void testStartphase_data();
    void testStartphase();
    void testStartName();
    void testServiceRegistered();
    void testRegisteringAndManipulatingANewService();
    void testRemovalOfNewServiceFile();
};

#endif // KAUTOSTARTTEST_H
