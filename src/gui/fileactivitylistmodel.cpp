/*
 * Copyright (C) by Felix Weilbach <felix.weilbach@nextcloud.com>
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

#include "fileactivitylistmodel.h"
#include "folderman.h"
#include "tray/ActivityListModel.h"

namespace OCC {

FileActivityListModel::FileActivityListModel(QObject *parent)
    : ActivityListModel(nullptr, parent)
{
    _displayActions = false;
}

void FileActivityListModel::load(AccountState *accountState, const QString &localPath)
{
    Q_ASSERT(accountState);
    if (!accountState) {
        return;
    }
    _accountState = accountState;

    const auto folder = FolderMan::instance()->folderForPath(localPath);
    if (!folder) {
        return;
    }

    const auto file = localPath.mid(folder->cleanPath().length() + 1);
    SyncJournalFileRecord fileRecord;
    if (!folder->journalDb()->getFileRecord(file, &fileRecord) || !fileRecord.isValid()) {
        return;
    }

    _fileId = fileRecord._fileId;
    slotRefreshActivity();
}

void FileActivityListModel::startFetchJob()
{
    if (!_accountState->isConnected()) {
        return;
    }

    const QString url(QStringLiteral("ocs/v2.php/apps/activity/api/v2/activity/filter"));
    auto job = new JsonApiJob(_accountState->account(), url, this);
    QObject::connect(job, &JsonApiJob::jsonReceived,
        this, &FileActivityListModel::activitiesReceived);

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("sort"), QStringLiteral("asc"));
    params.addQueryItem(QStringLiteral("object_type"), "files");
    params.addQueryItem(QStringLiteral("object_id"), _fileId);
    job->addQueryParams(params);
    _currentlyFetching = true;
    _doneFetching = true;
    _hideOldActivities = false;
    job->start();
}
}
