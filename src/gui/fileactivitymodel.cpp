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
#include <cstddef>
#include <iterator>
#include <tuple>
#include <algorithm>

#include <QAbstractItemModel>
#include <QDateTime>
#include <QPalette>
#include <QLoggingCategory>
#include <pushnotifications.h>
#include <theme.h>
#include <common/utility.h>

#include "fileactivitydialog.h"
#include "fileactivitymodel.h"
#include "activityjob.h"
#include "tray/ActivityListModel.h"

Q_LOGGING_CATEGORY(lcFileActivityModel, "nextcloud.gui.fileactivitymodel", QtInfoMsg)

namespace {
constexpr auto defaultActivityPollInterval = 20 * 1000;
}

namespace OCC {

FileActivity::FileActivity() = default;

FileActivity::FileActivity(int id, const QString &message, const QDateTime &timestamp, const Type type)
    : _id(id)
    , _message(message)
    , _timestamp(timestamp)
    , _type(type)
{
}

int FileActivity::id() const
{
    return _id;
}

QString FileActivity::message() const
{
    return _message;
}

void FileActivity::setMessage(const QString &message)
{
    _message = message;
}

QDateTime FileActivity::dateTime() const
{
    return _timestamp;
}

void FileActivity::setTimestamp(const QDateTime &dateTime)
{
    _timestamp = dateTime;
}

FileActivity::Type FileActivity::type() const
{
    return _type;
}

void FileActivity::setType(Type type)
{
    _type = type;
}

std::unordered_map<int, std::unordered_map<QString, std::shared_ptr<QPixmap>>> FileActivityListModel::iconCache;

static bool updateTimeAgo(FileActivityListModel::DisplayableFileActivity &fileActivity)
{
    const auto timeAgo = Utility::timeAgoInWords(fileActivity._dateTime.toLocalTime());

    if (fileActivity._timeAgo != timeAgo) {
        fileActivity._timeAgo = timeAgo;
        return true;
    }

    return false;
}

FileActivityListModel::FileActivityListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    _updateTimeAgoTimer.setInterval(defaultActivityPollInterval);
    connect(&_updateTimeAgoTimer, &QTimer::timeout, this, [this] {
        for (std::size_t i = 0; i < _fileActivities.size(); ++i) {
            auto &fileActivity = *_fileActivities[i];
            if (updateTimeAgo(fileActivity)) {
                const auto fileActivityIndex = index(static_cast<int>(i));
                emit dataChanged(fileActivityIndex, fileActivityIndex);
            }
        }
    });
    _updateTimeAgoTimer.start();
}

void FileActivityListModel::addFileActivity(const FileActivity &fileActivity)
{
    const auto fileActivitiesMapIter = _fileActivityMap.find(fileActivity.id());
    if (fileActivitiesMapIter != _fileActivityMap.end()) {
        // Update a already inserted activity
        const auto fileActivityToUpdate = std::get<1>(fileActivitiesMapIter->second);
        const auto fileActivityToUpdateIndex = std::get<0>(fileActivitiesMapIter->second);
        fileActivityToUpdate->_message = fileActivity.message();
        fileActivityToUpdate->_type = fileActivity.type();
        fileActivityToUpdate->_dateTime = fileActivity.dateTime();
        fileActivityToUpdate->_type = fileActivity.type();
        updateTimeAgo(*fileActivityToUpdate);
        emit dataChanged(index(fileActivityToUpdateIndex), index(fileActivityToUpdateIndex));
    } else {
        // Insert a new activity
        const auto rowIndex = rowCount();
        beginInsertRows(QModelIndex(), rowIndex, rowIndex);
        auto fileActivityPtr = std::make_unique<DisplayableFileActivity>();
        fileActivityPtr->_id = fileActivity.id();
        fileActivityPtr->_message = fileActivity.message();
        fileActivityPtr->_dateTime = fileActivity.dateTime();
        fileActivityPtr->_type = fileActivity.type();
        updateTimeAgo(*fileActivityPtr);
        _fileActivityMap[fileActivityPtr->_id] = std::make_tuple(_fileActivities.size(),
            fileActivityPtr.get());
        _fileActivities.emplace_back(std::move(fileActivityPtr));
        endInsertRows();
    }

    std::sort(_fileActivities.begin(), _fileActivities.end(),
        [](const std::unique_ptr<FileActivityListModel::DisplayableFileActivity> &fileActivityLeft,
            const std::unique_ptr<FileActivityListModel::DisplayableFileActivity> &fileActivityRight) {
            return fileActivityLeft->_dateTime < fileActivityRight->_dateTime;
        });
}

void FileActivityListModel::addFileActivities(const std::vector<FileActivity> &fileActivities)
{
    for (const auto &fileActivity : fileActivities) {
        addFileActivity(fileActivity);
    }
}

int FileActivityListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(_fileActivities.size());
}

QVariant FileActivityListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || _fileActivities.size() == 0 || role != Qt::DisplayRole) {
        return QVariant();
    }

    return QVariant::fromValue(*(_fileActivities[_fileActivities.size() - 1 - index.row()].get()));
}

QVariant FileActivityListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QAbstractListModel::headerData(section, orientation, role);
}

std::shared_ptr<QPixmap> FileActivityListModel::getIconPixmap(const QString &iconName, int size)
{
    const auto iconSizeIterator = iconCache.find(size);
    if (iconSizeIterator != iconCache.end()) {
        auto iconMap = iconSizeIterator->second;
        const auto iconIterator = iconMap.find(iconName);
        if (iconIterator != iconMap.end()) {
            return iconIterator->second;
        }

        QPalette palette;
        const auto isDarkBackground = Theme::isDarkColor(palette.window().color());
        const auto iconPixmap = std::make_shared<QPixmap>(Theme::instance()->uiThemeIcon(iconName, isDarkBackground).pixmap(size));
        iconMap[iconName] = iconPixmap;
        return iconPixmap;
    }

    iconCache[size] = std::unordered_map<QString, std::shared_ptr<QPixmap>>();
    return getIconPixmap(iconName, size);
}

std::shared_ptr<QPixmap> FileActivityListModel::pixmapForActivityType(FileActivity::Type type, int size)
{
    QPalette palette;
    const auto backgroundColor = palette.window().color();

    switch (type) {
    case FileActivity::Type::Changed:
        return getIconPixmap(QStringLiteral("change.svg"), size);
    case FileActivity::Type::Created:
        return getIconPixmap(QStringLiteral("add.svg"), size);
    case FileActivity::Type::Shared:
        return getIconPixmap(QStringLiteral("share.svg"), size);
    default:
        return getIconPixmap(QStringLiteral("activity.svg"), size);
    }
}

FileActivityDialogModel::FileActivityDialogModel(std::unique_ptr<ActivityJob> activityJob, QObject *parent)
    : QObject(parent)
    , _activityJob(std::move(activityJob))
{
    connect(_activityJob.get(), &OcsActivityJob::finished, this, &FileActivityDialogModel::activitiesReceived);
    connect(_activityJob.get(), &OcsActivityJob::error, this, &FileActivityDialogModel::onErrorFetchingActivities);

    _activitiesPollTimer.setInterval(defaultActivityPollInterval);
    connect(&_activitiesPollTimer, &QTimer::timeout, this, [this] {
        queryActivities();
    });
    _activitiesPollTimer.start();
}

void FileActivityDialogModel::start(const QString &fileId)
{
    _fileId = fileId;

    hideActivities();
    showProgress();
    queryActivities();
}

void FileActivityDialogModel::queryActivities()
{
    if (_fileId.isEmpty()) {
        return;
    }

    hideError();
    const QString type = "files";
    _activityJob->queryActivities(type, _fileId, _since);
}

FileActivityListModel *FileActivityDialogModel::getActivityListModel()
{
    return &_fileActivityListModel;
}

void FileActivityDialogModel::activitiesReceived(const std::vector<Activity> &activites)
{
    std::vector<FileActivity> fileActivities;
    std::transform(
        activites.cbegin(), activites.cend(), std::back_inserter(fileActivities), [&](const Activity &activity) {
            _since = static_cast<int>(activity._id);
            FileActivity::Type type;
            if (activity._fileAction == "file_created") {
                type = FileActivity::Type::Created;
            } else if (activity._fileAction == "file_changed") {
                type = FileActivity::Type::Changed;
            } else if (activity._fileAction == "shared") {
                type = FileActivity::Type::Shared;
            } else {
                qCWarning(lcFileActivityModel) << "Unknown file action" << activity._fileAction << "encountered";
                type = FileActivity::Type::Unknown;
            }
            return FileActivity(static_cast<int>(activity._id), activity._subject, activity._dateTime, type);
        });

    _fileActivityListModel.addFileActivities(fileActivities);
    showActivities();
    hideProgress();
}

void FileActivityDialogModel::onErrorFetchingActivities()
{
    hideProgress();
    emit showError(tr("Could not fetch activities"));
}

void FileActivityDialogModel::setActivityPollInterval(int interval)
{
    _activitiesPollTimer.setInterval(interval);
}
}
