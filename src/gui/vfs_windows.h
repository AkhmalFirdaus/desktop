/*
* Copyright(C) 2018 by AMCO
* Copyright(C) 2018 by Jonathan Ponciano <jponcianovera@ciencias.unam.mx>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
*(at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General Public License
* for more details.
*/

#ifndef VFS_WINDOWS_H
#define VFS_WINDOWS_H

#include "virtualdriveinterface.h"

#include "dokan.h"
#include "fileinfo.h"

#include "discoveryphase.h"
#include "accountstate.h"
#include "configfile.h"

#include <QMutex>
#include <QWaitCondition>
#include <QRunnable>
#include <QThreadPool>
#include <QStorageInfo>

namespace OCC {

class VfsWindows;

class CleanIgnoredTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit CleanIgnoredTask(VfsWindows *vfs);
    void run();

private:
    VfsWindows *_vfs;
};

class VfsWindows : public OCC::VirtualDriveInterface
{
    Q_OBJECT
public:
    explicit VfsWindows(AccountState *accountState, QObject *parent = nullptr);
    ~VfsWindows();
    void mount() override;
    void unmount() override;
    bool removeRecursively(const QString &dirName);

    QStringList* contentsOfDirectoryAtPath(QString path, QVariantMap &error);
    QList<QString> ignoredList;

    void createFileAtPath(QString path, QVariantMap &error);
    void moveFileAtPath(QString path, QString npath,QVariantMap &error);
    void createDirectoryAtPath(QString path, QVariantMap &error);
    void moveDirectoryAtPath(QString path, QString npath, QVariantMap &error);

    void openFileAtPath(QString path, QVariantMap &error);
    void writeFileAtPath(QString path, QVariantMap &error);
    void deleteFileAtPath(QString path, QVariantMap &error);
    void startDeleteDirectoryAtPath(QString path, QVariantMap &error);
    void endDeleteDirectoryAtPath(QString path, QVariantMap &error);

private:
    QMap<QString, OCC::DiscoveryDirectoryResult *> _fileListMap;
    QPointer<OCC::DiscoveryFolderFileList> _remotefileListJob;

    // To sync
    QMutex _mutex;
    QWaitCondition _dirCondition;

signals:
    void startRemoteFileListJob(QString path);

public slots:
    void folderFileListFinish(OCC::DiscoveryDirectoryResult *dr);
};

} // namespace OCC

#endif // VFS_WINDOWS_H
