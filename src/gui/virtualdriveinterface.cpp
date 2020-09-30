/*
 * Copyright (C) by Kevin Ottens <kevin.ottens@nextcloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "virtualdriveinterface.h"

#include <QStandardPaths>

#include "accountstate.h"
#include "account.h"
#include "filesystem.h"
#include "folderman.h"
#include "theme.h"

namespace {
bool ensureFolderExists(const QString &path)
{
    QDir dir(path);
    if (!dir.exists() && !dir.mkpath(".")) {
        return false;
    }
    OCC::FileSystem::setFolderMinimumPermissions(path);
    return true;
}
}

OCC::VirtualDriveInterface::VirtualDriveInterface(AccountState *accountState, QObject *parent)
    : QObject(parent)
{
    const auto user = accountState->account()->davUser();
    const auto host = accountState->account()->davUrl().host();
    const auto id = QString(user + '@' + host);
    _cachePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cachedFiles/" + id;
    _mountPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/" + OCC::Theme::instance()->appName() + "Volumes/" + id;

    if (!ensureFolderExists(_cachePath)) {
        qCritical() << "Couldn't create cache dir for VFS:" << _cachePath;
        return;
    }
    if (!ensureFolderExists(_mountPath)) {
        qCritical() << "Couldn't create mount dir for VFS:" << _mountPath;
        return;
    }

    const auto folder = FolderMan::instance()->folderForPath(_cachePath);
    if (!folder) {
        const auto folderDefinition = [=] {
            FolderDefinition d;
            d.localPath = FolderDefinition::prepareLocalPath(_cachePath);
            d.targetPath = FolderDefinition::prepareTargetPath(QString());
            d.ignoreHiddenFiles = FolderMan::instance()->ignoreHiddenFiles();
            return d;
        }();
        FolderMan::instance()->addFolder(accountState, folderDefinition);
    }
}

OCC::VirtualDriveInterface::~VirtualDriveInterface() = default;

QString OCC::VirtualDriveInterface::cachePath() const
{
    return _cachePath;
}

QString OCC::VirtualDriveInterface::mountPath() const
{
    return _mountPath;
}

QString OCC::VirtualDriveInterface::mapToCacheFilename(const QString &filename) const
{
    Q_ASSERT(filename.startsWith(mountPath()));
    if (filename.startsWith(mountPath())) {
        return cachePath() + filename.mid(mountPath().length());
    } else {
        return filename;
    }
}

QString OCC::VirtualDriveInterface::mapToMountFilename(const QString &filename) const
{
    Q_ASSERT(filename.startsWith(cachePath()));
    if (filename.startsWith(cachePath())) {
        return mountPath() + filename.mid(cachePath().length());
    } else {
        return filename;
    }
}
