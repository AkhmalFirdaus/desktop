import QtQuick.Window 2.15

import com.nextcloud.desktopclient 1.0 as NC

Window {
    id: dialog

    property NC.FileActivityListModel model: NC.FileActivityListModel { }
    
    width: 500
    height: 500

    ActivityList {
        anchors.fill: parent
        activityListModel: dialog.model
    }
}
