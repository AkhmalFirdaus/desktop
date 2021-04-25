#include "config.h"
#include "syncresult.h"


#include <QIcon>
#include <QMetaEnum>
#include <QString>
#include <QUrl>

namespace OCC {

class SyncResult;

/**
    * @brief IconTheme stores properties of an icon theme
    */
class OWNCLOUDSYNC_EXPORT IconTheme : public QObject
{
    Q_PROPERTY(QUrl stateOnlineImageSource READ stateOnlineImageSource CONSTANT)
    Q_PROPERTY(QUrl stateOfflineImageSource READ stateOfflineImageSource CONSTANT)
    Q_PROPERTY(QUrl stateOnlineImageSource READ stateOnlineImageSource CONSTANT)
    Q_PROPERTY(QUrl statusOnlineImageSource READ statusOnlineImageSource CONSTANT)
    Q_PROPERTY(QUrl statusDoNotDisturbImageSource READ statusDoNotDisturbImageSource CONSTANT)
    Q_PROPERTY(QUrl statusAwayImageSource READ statusAwayImageSource CONSTANT)
    Q_PROPERTY(QUrl statusInvisibleImageSource READ statusInvisibleImageSource CONSTANT)
#ifndef TOKEN_AUTH_ONLY
    Q_PROPERTY(QIcon folderDisabledIcon READ folderDisabledIcon CONSTANT)
    Q_PROPERTY(QIcon folderOfflineIcon READ folderOfflineIcon CONSTANT)
    Q_PROPERTY(QIcon applicationIcon READ applicationIcon CONSTANT)
#endif

public:

    /**
    * @brief IconFlavor lists the possible color themes for an icon theme
    */
    enum Flavor {Hicolor, White, Black};

    IconTheme(Flavor f)
    {
        flavor = f;
    };
    ~IconTheme();

    QString objectName() const;

    /**
    * @brief Returns the flavor of the systray icons
    */
    static Flavor pickFlavor(bool mono, bool systray);

    /**
     * @brief Returns full path to an online state icon
     * @return QUrl full path to an icon
     */
    QUrl stateOnlineImageSource() const;

    /**
     * @brief Returns full path to an offline state icon
     * @return QUrl full path to an icon
     */
    QUrl stateOfflineImageSource() const;
    
    /**
     * @brief Returns full path to an online user status icon
     * @return QUrl full path to an icon
     */
    QUrl statusOnlineImageSource() const;
    
    /**
     * @brief Returns full path to an do not disturb user status icon
     * @return QUrl full path to an icon
     */
    QUrl statusDoNotDisturbImageSource() const;
    
    /**
     * @brief Returns full path to an away user status icon
     * @return QUrl full path to an icon
     */
    QUrl statusAwayImageSource() const;
    
    /**
     * @brief Returns full path to an invisible user status icon
     * @return QUrl full path to an icon
     */
    QUrl statusInvisibleImageSource() const;

// #ifndef TOKEN_AUTH_ONLY
    QIcon iconByName(const QString &name);
// #endif

#ifndef TOKEN_AUTH_ONLY
    /**
      * get an sync state icon
      */
    virtual QIcon syncStateIcon(SyncResult::Status);

    virtual QIcon folderDisabledIcon();
    virtual QIcon folderOfflineIcon();
#endif


    /**
     * @brief Generates image path in the resources
     * @param name Name of the image file
     * @param size Size in the power of two (16, 32, 64, etc.)
     * @param sysTray Whether the image requested is for Systray or not
     * @return QString image path in the resources
     **/
    QString themeImagePath(const QString &name, int size = -1) const;


    /**
    * @brief Request suitable QIcon resource depending on the background colour of the parent widget.
    *
    * This should be replaced (TODO) by a real theming implementation for the client UI
    * (actually 2019/09/13 only systray theming).
    */
    virtual QIcon uiThemeIcon(const QString &iconName, bool uiHasDarkBg) const;

private:
    Flavor flavor;
#ifndef TOKEN_AUTH_ONLY
    mutable QHash<QString, QIcon> _iconCache;
    QPixmap loadPixmap(const QString &name, int size);
    static QVector<int> sizes();
    static void ubuntuHack(QPixmap px);
#endif
};

}