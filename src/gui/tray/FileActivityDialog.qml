import QtQml 2.15
import QtQuick 2.15
import QtQuick.Window 2.15

import com.nextcloud.desktopclient 1.0 as NC

Window {
    id: dialog

    property alias model: activityModel

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    NC.FileActivityListModel {
        id: activityModel
    }   

    width: 500
    height: 500

    ActivityList {
        anchors.fill: parent
        model: dialog.model
    }
}
