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

    static bool isDarkColor(const QColor &color);



    /**
     * @brief Creates a colour-aware icon based on the specified palette's base colour.
     *
     * @return QIcon, colour-aware (inverted on dark backgrounds).
     *
     * 2019/12/09: Moved here from SettingsDialog.
     */
    // static QIcon createColorAwareIcon(const QString &name, const QPalette &palette);

    /**
     * @brief Creates a colour-aware icon based on the app palette's base colour (Dark-/Light-Mode switching).
     *
     * @return QIcon, colour-aware (inverted on dark backgrounds).
     *
     * 2019/12/09: Moved here from SettingsDialog.
     */
    // static QIcon createColorAwareIcon(const QString &name);
};

#endif

}