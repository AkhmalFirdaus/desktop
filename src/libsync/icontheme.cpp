#include "common/utility.h"
#include "icontheme.h"

#include <QBitmap>
#include <QFile>
#include <QMetaEnum>
#include <QPainter>
#include <QSvgRenderer>

namespace {


QUrl imagePathToUrl(const QString &imagePath)
{
    if (imagePath.startsWith(':')) {
        auto url = QUrl();
        url.setScheme(QStringLiteral("qrc"));
        url.setPath(imagePath.mid(1));
        return url;
    } else {
        return QUrl::fromLocalFile(imagePath);
    }
}
}

namespace OCC {

QString IconTheme::objectName() const {
    return QMetaEnum::fromType<IconTheme::Flavor>().valueToKey((int) this->flavor);
}

IconTheme::Flavor IconTheme::pickFlavor(bool mono, bool systray)
{
    if(systray) {
        if (mono && Utility::hasDarkSystray()) {
            return IconTheme::Flavor::White;
        } else if (mono) {
            return IconTheme::Flavor::Black;
        }
    }
    return IconTheme::Flavor::Hicolor;
}

QUrl IconTheme::stateOnlineImageSource() const
{
    return imagePathToUrl(this->themeImagePath("state-ok"));
}

QUrl IconTheme::stateOfflineImageSource() const
{
    return imagePathToUrl(this->themeImagePath("state-offline", 16));
}

QUrl IconTheme::statusOnlineImageSource() const
{
    return imagePathToUrl(this->themeImagePath("user-status-online", 16));
}

QUrl IconTheme::statusDoNotDisturbImageSource() const
{
    return imagePathToUrl(this->themeImagePath("user-status-dnd", 16));
}

QUrl IconTheme::statusAwayImageSource() const
{
    return imagePathToUrl(this->themeImagePath("user-status-away", 16));
}

QUrl IconTheme::statusInvisibleImageSource() const
{
    return imagePathToUrl(this->themeImagePath("user-status-invisible", 16));
}

QIcon IconTheme::syncStateIcon(SyncResult::Status status)
{
    // FIXME: Mind the size!
    QString statusIcon;

    switch (status) {
    case SyncResult::Undefined:
        // this can happen if no sync connections are configured.
        statusIcon = QLatin1String("state-warning");
        break;
    case SyncResult::NotYetStarted:
    case SyncResult::SyncRunning:
        statusIcon = QLatin1String("state-sync");
        break;
    case SyncResult::SyncAbortRequested:
    case SyncResult::Paused:
        statusIcon = QLatin1String("state-pause");
        break;
    case SyncResult::SyncPrepare:
    case SyncResult::Success:
        statusIcon = QLatin1String("state-ok");
        break;
    case SyncResult::Problem:
        statusIcon = QLatin1String("state-warning");
        break;
    case SyncResult::Error:
    case SyncResult::SetupError:
    // FIXME: Use state-problem once we have an icon.
    default:
        statusIcon = QLatin1String("state-error");
    }

    return this->iconByName(statusIcon);
}

QIcon IconTheme::folderDisabledIcon()
{
    return this->iconByName(QLatin1String("state-pause"));
}

QIcon IconTheme::folderOfflineIcon()
{
    return this->iconByName(QLatin1String("state-offline"));
}

/*
* helper to load a icon from either the icon theme the desktop provides or from
* the apps Qt resources.
*/
QIcon IconTheme::iconByName(const QString &name)
{
    QIcon &cached = _iconCache[name + "," + this->objectName()];
    if (cached.isNull()) {
        if (QIcon::hasThemeIcon(name)) {
            // use from theme
            return cached = QIcon::fromTheme(name);
        }

        const auto svgName = QString::fromLatin1(":/client/theme/%1/%2.svg").arg(flavor).arg(name);
        QSvgRenderer renderer(svgName);
        const auto createPixmapFromSvg = [&renderer] (int size) {
            QImage img(size, size, QImage::Format_ARGB32);
            img.fill(Qt::GlobalColor::transparent);
            QPainter imgPainter(&img);
            renderer.render(&imgPainter);
            return QPixmap::fromImage(img);
        };

        for (int size : sizes()) {
            QPixmap px;
            if (USE_SVG) {
                px = createPixmapFromSvg(size);
            } else {
                px = loadPixmap(name, size);
            }
            if (px.isNull()) continue;

            ubuntuHack(px);

            cached.addPixmap(px);
        }
    }

#ifdef Q_OS_MAC
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    // This defines the icon as a template and enables automatic macOS color handling
    // See https://bugreports.qt.io/browse/QTBUG-42109
    cached.setIsMask(this->flavor == Flavor::White || this->flavor == Flavor::Black);
#endif
#endif

    return cached;
}


QString IconTheme::themeImagePath(const QString &name, int size) const
{

    // branded client may have several sizes of the same icon
    const QString filePath = (USE_SVG || size <= 0)
            ? QString::fromLatin1(":/client/theme/%1/%2").arg(this->objectName()).arg(name)
            : QString::fromLatin1(":/client/theme/%1/%2-%3").arg(this->objectName()).arg(name).arg(size);

    const QString svgPath = filePath + ".svg";
    if (USE_SVG) {
        return svgPath;
    }

    const QString pngPath = filePath + ".png";
    // Use the SVG as fallback if a PNG is missing so that we get a chance to display something
    if (QFile::exists(pngPath)) {
        return pngPath;
    } else {
        return svgPath;
    }

}

QIcon IconTheme::uiThemeIcon(const QString &iconName, bool uiHasDarkBg) const
{
    QString themeResBasePath = ":/client/theme/";
    QString iconPath = themeResBasePath + (uiHasDarkBg?"white/":"black/") + iconName;
    std::string icnPath = iconPath.toUtf8().constData();
    return QIcon(QPixmap(iconPath));
}

QPixmap IconTheme::loadPixmap (const QString &name, int size) {
    return QPixmap(QString::fromLatin1(":/client/theme/%1/%2-%3.png").arg(this->objectName()).arg(name).arg(size));
}

QVector<int> IconTheme::sizes() {
    return USE_SVG ? QVector<int>{ 16, 32, 64, 128, 256 }
                  : QVector<int>{ 16, 22, 32, 48, 64, 128, 256, 512, 1024 };
}

void IconTheme::ubuntuHack(QPixmap px) {
    // HACK, get rid of it by supporting FDO icon themes, this is really just emulating ubuntu-mono
    if (qgetenv("DESKTOP_SESSION") == "ubuntu") {
        QBitmap mask = px.createMaskFromColor(Qt::white, Qt::MaskOutColor);
        QPainter p(&px);
        p.setPen(QColor("#dfdbd2"));
        p.drawPixmap(px.rect(), mask, mask.rect());
    }
}


}
