// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2024-2026 Harald Sitter <sitter@kde.org>

#include "kdesktopfilefactory.h"

#include <filesystem>

#include <QDebug>
#include <QHash>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace
{

// $prefix-$filename
using DesktopFileID = QString;
// A Path!
using Path = QString;
// The dir prefix (e.g. when iterating applications/kde/foo.desktop it'd be 'kde')
using DesktopFilePrefix = QString;
// The output hash
using DesktopFileIDToPathHash = QHash<DesktopFileID, Path>;

[[nodiscard]] DesktopFileIDToPathHash build()
{
    DesktopFileIDToPathHash hash;
    for (const auto &dir : QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation)) {
        QList<std::pair<Path, DesktopFilePrefix>> searchPaths{{dir, {}}};
        for (const auto &[path, prefix] : searchPaths) {
            std::error_code errorCode;
            for (auto const &entry : std::filesystem::directory_iterator{path.toStdString(), errorCode}) {
                const auto filename = QString::fromStdString(entry.path().filename().string());
                const auto pathname = QString::fromStdString(entry.path().string());
                if (filename.endsWith(".desktop"_L1)) {
                    const DesktopFileID desktopFileID = prefix + filename;
                    if (!hash.contains(desktopFileID)) {
                        hash.insert(desktopFileID, pathname);
                    }
                } else if (entry.is_directory()) {
                    const QString dirPrefix = prefix + filename + '-'_L1;
                    searchPaths.emplace_back(pathname, dirPrefix);
                }
            }
            if (errorCode) {
                qWarning() << "Error iterating" << path << ":" << QString::fromStdString(errorCode.message());
            }
        }
    }
    return hash;
};

} // namespace

class KDesktopFileFactoryPrivate
{
};

KDesktopFileFactory::KDesktopFileFactory(QObject *parent)
    : QObject(parent)
    , d(new KDesktopFileFactoryPrivate)
{
    QMetaObject::invokeMethod(this, &KDesktopFileFactory::stateChanged, Qt::QueuedConnection);
}

KDesktopFileFactory::~KDesktopFileFactory() = default;

KDesktopFileFactory::State KDesktopFileFactory::state() const
{
    return State::Loaded;
}

QList<std::shared_ptr<KDesktopFile>> KDesktopFileFactory::allDesktopFiles()
{
    const auto hash = build();

    QList<std::shared_ptr<KDesktopFile>> desktopFiles;
    desktopFiles.reserve(hash.size());
    for (const auto &[desktopFileID, path] : hash.asKeyValueRange()) {
        // TODO: this is technically not spec compatible because we need to remember the prefix. currently kdesktopfile doesn't allow us to do that
        desktopFiles.emplace_back(std::make_shared<KDesktopFile>(path));
    }
    return desktopFiles;
}
