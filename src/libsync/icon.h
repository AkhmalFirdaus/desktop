#include <QIcon>
#include <QPalette>
#include <QString>

namespace OCC {

#ifndef ICON_H
#define ICON_H

class Icon : QIcon {

public:
    static QIcon fromTheme(const QString &name);

    static QIcon fromTheme(const QString &name, const QIcon &fallback);

    static QIcon fromTheme(const QString &name, const QPalette &palette);

    static QIcon createColorAwareIcon(const QString &name);

    static bool isDarkColor(const QColor &color);
};

#endif

}