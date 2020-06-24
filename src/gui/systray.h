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

namespace OCC {

#ifdef Q_OS_OSX
bool canOsXSendUserNotification();
void sendOsXUserNotification(const QString &title, const QString &message);
#endif

namespace Ui {
    class Systray;
}

/**
 * @brief The Systray class
 * @ingroup gui
 */
class Systray
    : public QSystemTrayIcon
{
    Q_OBJECT
public:
    static Systray *instance();
    virtual ~Systray() {};

    void setTrayEngine(QQmlApplicationEngine *trayEngine);
    void create();
    void showMessage(const QString &title, const QString &message, MessageIcon icon = Information);
    void setToolTip(const QString &tip);
    bool isOpen();

    Q_INVOKABLE void pauseResumeSync();
    Q_INVOKABLE bool syncIsPaused();
    Q_INVOKABLE void setOpened();
    Q_INVOKABLE void setClosed();
    Q_INVOKABLE void setPosition(QQuickWindow *window) const;

signals:
    void currentUserChanged();
    void openSettings();
    void openHelp();
    void shutdown();
    void pauseSync();
    void resumeSync();

    Q_INVOKABLE void hideWindow();
    Q_INVOKABLE void showWindow();
    Q_INVOKABLE void openShareDialog(const QString &sharePath, const QString &localPath);

public slots:
    void slotNewUserSelected();

private:
    static Systray *_instance;
    Systray();

    QPoint calcTrayIconCenter() const;
    struct Geometry;

    bool _isOpen = false;
    bool _syncIsPaused = false;
    QPointer<QQmlApplicationEngine> _trayEngine;
};

struct TaskBarPosition;

struct Systray::Geometry
{
    Geometry(QQuickWindow *window, QPoint iconCenter);
    void operator()();

protected:
    static QScreen *currentScreen();
    QPoint computeWindowPosition() const;

    TaskBarPosition *initTaskbarPosition() const;
    QRect taskbarGeometry() const;
    static QRect currentScreenRect();
    QPoint computeWindowReferencePoint() const;

    std::unique_ptr<TaskBarPosition> _taskbarPosition;
    QQuickWindow *_window;
    const QPoint _iconCenter;
};

} // namespace OCC

#endif //SYSTRAY_H
