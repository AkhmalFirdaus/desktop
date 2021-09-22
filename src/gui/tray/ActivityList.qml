import QtQuick 2.15
import QtQuick.Controls 2.15

import Style 1.0

import com.nextcloud.desktopclient 1.0 as NC

ListView {
    id: activityList

    signal showFileActivity(string displayPath, string absolutePath)
    signal activityItemClicked(int index)

    ScrollBar.vertical: ScrollBar {
        id: listViewScrollbar
    }

    keyNavigationEnabled: true

    Accessible.role: Accessible.List
    Accessible.name: qsTr("Activity list")

    delegate: ActivityItem {
        width: activityList.width
        height: Style.trayWindowHeaderHeight
        flickable: activityList
        onClicked: activityItemClicked(model.index)
        onFileActivityButtonClicked: showFileActivity(displayPath, absolutePath)
    }
}
