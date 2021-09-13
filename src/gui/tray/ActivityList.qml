import QtQuick 2.15
import QtQuick.Controls 2.15

import Style 1.0

import com.nextcloud.desktopclient 1.0 as NC

ListView {
    id: activityList

    property NC.ActivityListModel activityListModel;
    readonly property int maxActionButtons: 2

    signal showFileActivity(string displayPath, string absolutePath)

    clip: true
    ScrollBar.vertical: ScrollBar {
        id: listViewScrollbar
    }

    keyNavigationEnabled: true

    Accessible.role: Accessible.List
    Accessible.name: qsTr("Activity list")

    model: activityListModel

    delegate: ActivityItem {
        width: activityList.width
        height: Style.trayWindowHeaderHeight
        onClicked: activityListModel.triggerDefaultAction(model.index)
        onFileActivityButtonClicked: showFileActivity(displayPath, absolutePath)
    }
}
