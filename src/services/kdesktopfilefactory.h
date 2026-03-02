// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#pragma once

#include <memory>

#include <KDesktopFile>

#include "kservice_export.h"

class KSERVICE_EXPORT KDesktopFileFactory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
public:
    enum class State {
        Loading,
        Loaded,
    };
    Q_ENUM(State)

    explicit KDesktopFileFactory(QObject *parent = nullptr);
    ~KDesktopFileFactory() override;
    Q_DISABLE_COPY_MOVE(KDesktopFileFactory)

    [[nodiscard]] State state() const;
    [[nodiscard]] QList<std::shared_ptr<KDesktopFile>> allDesktopFiles();

Q_SIGNALS:
    void stateChanged();

private:
    std::unique_ptr<class KDesktopFileFactoryPrivate> d;
};
