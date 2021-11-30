/*
 * Copyright (C) by Cédric Bellegarde <gnumdk@gmail.com>
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

#ifndef SYSTRAY_H
#define SYSTRAY_H

#include <QSystemTrayIcon>

#include "accountmanager.h"
#include "tray/UserModel.h"

class QScreen;
class QQmlApplicationEngine;
class QQuickWindow;
class QWindow;
class QQuickWindow;

namespace OCC {

#ifdef Q_OS_OSX
bool canOsXSendUserNotification();
void sendOsXUserNotification(const QString &title, const QString &message);
void setTrayWindowLevelAndVisibleOnAllSpaces(QWindow *window);
#endif

/**
 * @brief The Systray class
 * @ingroup gui
 */
class Systray
    : public QSystemTrayIcon
{
    Q_OBJECT

    Q_PROPERTY(QString windowTitle READ windowTitle CONSTANT)
    Q_PROPERTY(bool useNormalWindow READ useNormalWindow CONSTANT)

public:
    static Systray *instance();
    ~Systray() override = default;

    enum class TaskBarPosition { Bottom, Left, Top, Right };
    Q_ENUM(TaskBarPosition);

    void setTrayEngine(QQmlApplicationEngine *trayEngine);
    void create();
    void showMessage(const QString &title, const QString &message, MessageIcon icon = Information);
    void setToolTip(const QString &tip);
    bool isOpen();
    QString windowTitle() const;
    bool useNormalWindow() const;

    Q_INVOKABLE void pauseResumeSync();
    Q_INVOKABLE bool syncIsPaused();
    Q_INVOKABLE void setOpened();
    Q_INVOKABLE void setClosed();
    Q_INVOKABLE void positionWindow(QQuickWindow *window) const;
    Q_INVOKABLE void forceWindowInit(QQuickWindow *window) const;

signals:
    void currentUserChanged();
    void openAccountWizard();
    void openMainDialog();
    void openSettings();
    void openHelp();
    void shutdown();

    void hideWindow();
    void showWindow();
    void openShareDialog(const QString &sharePath, const QString &localPath);

public slots:
    void slotNewUserSelected();

private slots:
    void slotUnpauseAllFolders();
    void slotPauseAllFolders();

private:
    void setPauseOnAllFoldersHelper(bool pause);

    static Systray *_instance;
    Systray();

    QScreen *currentScreen() const;
    QRect currentScreenRect() const;
    QPoint computeWindowReferencePoint() const;
    QPoint calcTrayIconCenter() const;
    TaskBarPosition taskbarOrientation() const;
    QRect taskbarGeometry() const;
    QPoint computeWindowPosition(int width, int height) const;

    bool _isOpen = false;
    bool _syncIsPaused = true;
    QPointer<QQmlApplicationEngine> _trayEngine;
};

} // namespace OCC

#endif //SYSTRAY_H
