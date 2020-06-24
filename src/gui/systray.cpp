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

#include "accountmanager.h"
#include "systray.h"
#include "theme.h"
#include "config.h"
#include "common/utility.h"
#include "tray/UserModel.h"

#include <QCursor>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QScreen>

#ifdef USE_FDO_NOTIFICATIONS
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCall>
#define NOTIFICATIONS_SERVICE "org.freedesktop.Notifications"
#define NOTIFICATIONS_PATH "/org/freedesktop/Notifications"
#define NOTIFICATIONS_IFACE "org.freedesktop.Notifications"
#endif

namespace OCC {

Q_LOGGING_CATEGORY(lcSystray, "nextcloud.gui.systray")

Systray *Systray::_instance = nullptr;

Systray *Systray::instance()
{
    if (!_instance) {
        _instance = new Systray();
    }
    return _instance;
}

void Systray::setTrayEngine(QQmlApplicationEngine *trayEngine)
{
    _trayEngine = trayEngine;

    _trayEngine->addImportPath("qrc:/qml/theme");
    _trayEngine->addImageProvider("avatars", new ImageProvider);
}

Systray::Systray()
    : QSystemTrayIcon(nullptr)
{
    qmlRegisterSingletonType<UserModel>("com.nextcloud.desktopclient", 1, 0, "UserModel",
        [](QQmlEngine *, QJSEngine *) -> QObject * {
            return UserModel::instance();
        }
    );

    qmlRegisterSingletonType<UserAppsModel>("com.nextcloud.desktopclient", 1, 0, "UserAppsModel",
        [](QQmlEngine *, QJSEngine *) -> QObject * {
            return UserAppsModel::instance();
        }
    );

    qmlRegisterSingletonType<Systray>("com.nextcloud.desktopclient", 1, 0, "Systray",
        [](QQmlEngine *, QJSEngine *) -> QObject * {
            return Systray::instance();
        }
    );

    connect(UserModel::instance(), &UserModel::newUserSelected,
        this, &Systray::slotNewUserSelected);

    connect(AccountManager::instance(), &AccountManager::accountAdded,
        this, &Systray::showWindow);
}

void Systray::create()
{
    if (_trayEngine) {
        if (!AccountManager::instance()->accounts().isEmpty()) {
            _trayEngine->rootContext()->setContextProperty("activityModel", UserModel::instance()->currentActivityModel());
        }
        _trayEngine->load(QStringLiteral("qrc:/qml/src/gui/tray/Window.qml"));
    }
    hideWindow();
    emit activated(QSystemTrayIcon::ActivationReason::Unknown);
}

void Systray::slotNewUserSelected()
{
    if (_trayEngine) {
        // Change ActivityModel
        _trayEngine->rootContext()->setContextProperty("activityModel", UserModel::instance()->currentActivityModel());
    }

    // Rebuild App list
    UserAppsModel::instance()->buildAppList();
}

bool Systray::isOpen()
{
    return _isOpen;
}

Q_INVOKABLE void Systray::setOpened()
{
    _isOpen = true;
}

Q_INVOKABLE void Systray::setClosed()
{
    _isOpen = false;
}

void Systray::showMessage(const QString &title, const QString &message, MessageIcon icon)
{
#ifdef USE_FDO_NOTIFICATIONS
    if (QDBusInterface(NOTIFICATIONS_SERVICE, NOTIFICATIONS_PATH, NOTIFICATIONS_IFACE).isValid()) {
        const QVariantMap hints = {{QStringLiteral("desktop-entry"), LINUX_APPLICATION_ID}};
        QList<QVariant> args = QList<QVariant>() << APPLICATION_NAME << quint32(0) << APPLICATION_ICON_NAME
                                                 << title << message << QStringList() << hints << qint32(-1);
        QDBusMessage method = QDBusMessage::createMethodCall(NOTIFICATIONS_SERVICE, NOTIFICATIONS_PATH, NOTIFICATIONS_IFACE, "Notify");
        method.setArguments(args);
        QDBusConnection::sessionBus().asyncCall(method);
    } else
#endif
#ifdef Q_OS_OSX
        if (canOsXSendUserNotification()) {
        sendOsXUserNotification(title, message);
    } else
#endif
    {
        QSystemTrayIcon::showMessage(title, message, icon);
    }
}

void Systray::setToolTip(const QString &tip)
{
    QSystemTrayIcon::setToolTip(tr("%1: %2").arg(Theme::instance()->appNameGUI(), tip));
}

bool Systray::syncIsPaused()
{
    return _syncIsPaused;
}

void Systray::pauseResumeSync()
{
    if (_syncIsPaused) {
        _syncIsPaused = false;
        emit resumeSync();
    } else {
        _syncIsPaused = true;
        emit pauseSync();
    }
}

/********************************************************************************************/
/* Helper functions for cross-platform tray icon position and taskbar orientation detection */
/********************************************************************************************/

struct TaskBarPosition
{
    TaskBarPosition() = default;
    virtual ~TaskBarPosition() = default;
    static constexpr int spacing = 4;
    virtual QString name() const = 0;
    virtual QRect defaultDimensions(QRect screenRect, int pixels) const = 0;
    virtual QPoint windowReferencePoint(QPoint iconCenter, QRect screenRect, QRect panelRect) const = 0;
    virtual QPoint topLeft(QPoint reference, int width, int height) const = 0;
};

struct TaskBarPositionBottom : public TaskBarPosition
{
    QString name() const override { return "bottom"; }
    QRect defaultDimensions(QRect screenRect, int pixels) const override { return QRect(0, 0, screenRect.width(), pixels); }
    QPoint topLeft(QPoint reference, int width, int height) const override { return reference - QPoint(width / 2, height); }

    QPoint windowReferencePoint(QPoint iconCenter, QRect screenRect, QRect panelRect) const override
    {
        return {
            iconCenter.x(),
            screenRect.bottom() - panelRect.height() - spacing
        };
    }
};

struct TaskBarPositionTop : public TaskBarPosition
{
    QString name() const override { return "top"; }
    QRect defaultDimensions(QRect screenRect, int pixels) const override { return QRect(0, 0, screenRect.width(), pixels); }
    QPoint topLeft(QPoint reference, int width, int) const override { return reference - QPoint(width / 2, 0); }

    QPoint windowReferencePoint(QPoint iconCenter, QRect screenRect, QRect panelRect) const override
    {
        return {
            iconCenter.x(),
            screenRect.top() + panelRect.height() + spacing
        };
    }
};

struct TaskBarPositionLeft : public TaskBarPosition
{
    QString name() const override { return "left"; }
    QRect defaultDimensions(QRect screenRect, int pixels) const override { return QRect(0, 0, pixels, screenRect.height()); }
    QPoint topLeft(QPoint reference, int, int) const override { return reference; }

    QPoint windowReferencePoint(QPoint iconCenter, QRect screenRect, QRect panelRect) const override
    {
        return {
            screenRect.left() + panelRect.width() + spacing,
            iconCenter.y()
        };
    }
};

struct TaskBarPositionRight : public TaskBarPosition
{
    QString name() const override { return "right"; }
    QRect defaultDimensions(QRect screenRect, int pixels) const override { return QRect(0, 0, pixels, screenRect.height()); }
    QPoint topLeft(QPoint reference, int width, int) const override { return reference - QPoint(width, 0); }

    QPoint windowReferencePoint(QPoint iconCenter, QRect screenRect, QRect panelRect) const override
    {
        return {
            screenRect.right() - panelRect.width() - spacing,
            iconCenter.y()
        };
    }
};

void Systray::setPosition(QQuickWindow *window) const
{
    Geometry setWindowPosition(window, calcTrayIconCenter());
    setWindowPosition();
}

Systray::Geometry::Geometry(QQuickWindow *window, QPoint iconCenter)
    : _window(window)
    , _iconCenter(iconCenter)
    , _taskbarPosition(initTaskbarPosition())
{
}

void Systray::Geometry::operator()()
{
    _window->setScreen(currentScreen());

    auto position = computeWindowPosition();
    _window->setX(position.x());
    _window->setY(position.y());
}

QScreen *Systray::Geometry::currentScreen()
{
    const auto screens = QGuiApplication::screens();
    const auto cursorPos = QCursor::pos();

    for (const auto screen : screens) {
        if (screen->geometry().contains(cursorPos)) {
            return screen;
        }
    }

    return nullptr;
}

TaskBarPosition *Systray::Geometry::initTaskbarPosition() const
{
// macOS: Always on top
#if defined(Q_OS_MACOS)
    return new TaskBarPositionTop;
// Windows: Check registry for actual taskbar orientation
#elif defined(Q_OS_WIN)
    auto taskbarPosition = Utility::registryGetKeyValue(HKEY_CURRENT_USER,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3",
        "Settings");
    switch (taskbarPosition.toInt()) {
    // Mapping windows binary value (0 = left, 1 = top, 2 = right, 3 = bottom) to qml logic (0 = bottom, 1 = left...)
    case 0:
        return new TaskBarPositionLeft;
    case 1:
        return new TaskBarPositionTop;
    case 2:
        return new TaskBarPositionRight;
    case 3:
        return new TaskBarPositionBottom;
    default:
        return new TaskBarPositionBottom;
    }
// Probably Linux
#else
    const auto screenRect = currentScreenRect();

    const auto distBottom = screenRect.bottom() - _iconCenter.y();
    const auto distRight = screenRect.right() - _iconCenter.x();
    const auto distLeft = _iconCenter.x() - screenRect.left();
    const auto distTop = _iconCenter.y() - screenRect.top();

    const auto minDist = std::min({distRight, distTop, distBottom});

    if (minDist == distBottom) {
        return new TaskBarPositionBottom;
    } else if (minDist == distLeft) {
        return new TaskBarPositionLeft;
    } else if (minDist == distTop) {
        return new TaskBarPositionTop;
    } else {
        return new TaskBarPositionRight;
    }
#endif
}

QRect Systray::Geometry::taskbarGeometry() const
{
#if defined(Q_OS_WIN)
    QRect tbRect = Utility::getTaskbarDimensions();
    //QML side expects effective pixels, convert taskbar dimensions if necessary
    auto pixelRatio = currentScreen()->devicePixelRatio();
    if (pixelRatio != 1) {
        tbRect.setHeight(tbRect.height() / pixelRatio);
        tbRect.setWidth(tbRect.width() / pixelRatio);
    }
    return tbRect;
#elif defined(Q_OS_MACOS)
    // Finder bar is always 22px height on macOS (when treating as effective pixels)
    return _taskbarPosition->defaultDimensions(currentScreenRect(), 22)
#else
    // TODO: Get real taskbar dimensions Linux as well; 32px is just a guess
    return _taskbarPosition->defaultDimensions(currentScreenRect(), 32);
#endif
}

QRect Systray::Geometry::currentScreenRect()
{
    const auto screen = currentScreen();
    const auto rect = screen->geometry();
    return rect.translated(screen->virtualGeometry().topLeft());
}

QPoint Systray::Geometry::computeWindowReferencePoint() const
{
    const auto taskbarRect = taskbarGeometry();
    const auto screenRect = currentScreenRect();

    qCDebug(lcSystray) << "screenRect:" << screenRect;
    qCDebug(lcSystray) << "taskbarRect:" << taskbarRect;
    qCDebug(lcSystray) << "taskbarPosition:" << _taskbarPosition->name();
    qCDebug(lcSystray) << "trayIconCenter:" << _iconCenter;

    return _taskbarPosition->windowReferencePoint(_iconCenter, screenRect, taskbarRect);
}

QPoint Systray::Geometry::computeWindowPosition() const
{
    const auto referencePoint = computeWindowReferencePoint();
    const auto screenRect = currentScreenRect();

    const auto width = _window->width();
    const auto height = _window->height();

    const auto topLeft = _taskbarPosition->topLeft(referencePoint, width, height);
    const auto bottomRight = topLeft + QPoint(width, height);
    const auto windowRect = [=]() {
        const auto rect = QRect(topLeft, bottomRight);
        auto offset = QPoint();

        if (rect.left() < screenRect.left()) {
            offset.setX(screenRect.left() - rect.left() + 4);
        } else if (rect.right() > screenRect.right()) {
            offset.setX(screenRect.right() - rect.right() - 4);
        }

        if (rect.top() < screenRect.top()) {
            offset.setY(screenRect.top() - rect.top() + 4);
        } else if (rect.bottom() > screenRect.bottom()) {
            offset.setY(screenRect.bottom() - rect.bottom() - 4);
        }

        return rect.translated(offset);
    }();

    qCDebug(lcSystray) << "taskbarPosition:" << _taskbarPosition->name();
    qCDebug(lcSystray) << "screenRect:" << screenRect;
    qCDebug(lcSystray) << "windowRect (reference)" << QRect(topLeft, bottomRight);
    qCDebug(lcSystray) << "windowRect (adjusted )" << windowRect;

    return windowRect.topLeft();
}

QPoint Systray::calcTrayIconCenter() const
{
    QRect iconRect = geometry();

    if (iconRect == QRect()) {
        // QSystemTrayIcon::geometry() is broken for ages on most Linux DEs (default
        // QRect returned), thus fall back to mouse position (assuming tray icon is
        // activated by mouse click)
        return QCursor::pos();
    }

    return iconRect.center();
}

} // namespace OCC
