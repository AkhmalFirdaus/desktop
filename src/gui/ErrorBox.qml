import QtQuick 2.12

import Style 1.0

Item {
    id: errorBox
    
    property var text: ""

    property color color: Style.errorBoxTextColor
    property color backgroundColor: Style.errorBoxBackgroundColor
    property color borderColor: Style.errorBoxBorderColor
    
    implicitHeight: errorMessage.implicitHeight + 2 * 8

    Rectangle {
        anchors.fill: parent
        color: errorBox.backgroundColor
        border.color: errorBox.borderColor
    }

    Text {
        id: errorMessage
        
        anchors.fill: parent
        anchors.margins: 8
        width: parent.width
        color: errorBox.color
        wrapMode: Text.WordWrap
        text: errorBox.text
    }
}
