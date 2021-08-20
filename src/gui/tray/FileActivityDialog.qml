import QtQuick.Window 2.15

import com.nextcloud.desktopclient 1.0 as NC

Window {
    id: dialog
    
    width: 500
    height: 500

    property NC.FileActivityListModel model: NC.FileActivityListModel { }

    ActivityList {
        anchors.fill: parent
        activityListModel: dialog.model
    }
}
