#include "icon.h"
#include "theme.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QGuiApplication>

namespace OCC {

QIcon Icon::fromTheme(const QString &name)
{
    if (Theme::instance()->systrayUseMonoIcons()) {
        return Icon::fromTheme(name + "-symbolic", QGuiApplication::palette());
    } else {
        return QIcon::fromTheme(name);
    }

}

QIcon Icon::fromTheme(const QString &name, const QIcon &fallback)
{
    if (Theme::instance()->systrayUseMonoIcons()) {
        return Icon::fromTheme(name + "-symbolic", QGuiApplication::palette());
        // return Icon::fromTheme(name + "-symbolic", QGuiApplication::palette(), fallback)
    } else {
        return QIcon::fromTheme(name, fallback);
    }

}


// QIcon Icon::fromTheme(const QString &name, const QPalette &palette, const QIcon &fallback)

QIcon Icon::fromTheme(const QString &name, const QPalette &palette)
{
    QSvgRenderer renderer(name);
    QImage img(64, 64, QImage::Format_ARGB32);
    img.fill(Qt::GlobalColor::transparent);
    QPainter imgPainter(&img);
    QImage inverted(64, 64, QImage::Format_ARGB32);
    inverted.fill(Qt::GlobalColor::transparent);
    QPainter invPainter(&inverted);

    renderer.render(&imgPainter);
    renderer.render(&invPainter);

    inverted.invertPixels(QImage::InvertRgb);

    QIcon icon;
    if (Icon::isDarkColor(palette.color(QPalette::Base))) {
        icon.addPixmap(QPixmap::fromImage(inverted));
    } else {
        icon.addPixmap(QPixmap::fromImage(img));
    }
    if (Icon::isDarkColor(palette.color(QPalette::HighlightedText))) {
        icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    } else {
        icon.addPixmap(QPixmap::fromImage(inverted), QIcon::Normal, QIcon::On);
    }
    return icon;
}

bool Icon::isDarkColor(const QColor &color)
{
    // account for different sensitivity of the human eye to certain colors
    double threshold = 1.0 - (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()) / 255.0;
    return threshold > 0.5;
}

}