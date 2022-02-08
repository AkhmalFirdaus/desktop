import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Custom qml modules are in /theme (and included by resources.qrc)
import Style 1.0
import com.nextcloud.desktopclient 1.0

MenuItem {
    id: userLine
    height: Style.trayWindowHeaderHeight

    Accessible.role: Accessible.MenuItem
    Accessible.name: qsTr("Account entry")

    property variant dialog;
    property variant comp;
    activeFocusOnTab: false

    signal showUserStatusSelectorDialog(int id)

    RowLayout {
        id: userLineLayout
        spacing: 0
        anchors.fill: parent

        Button {
            id: accountButton
            Layout.preferredWidth: (userLineLayout.width * (5/6))
            Layout.preferredHeight: (userLineLayout.height)
            display: AbstractButton.IconOnly
            hoverEnabled: true
            flat: true

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Switch to account") + " " + name

            onClicked: if (!isCurrentUser) {
                UserModel.switchCurrentUser(id)
            } else {
                accountMenu.close()
            }

            background: Item {
                height: parent.height
                width: userLine.menu ? userLine.menu.width : 0
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    color: parent.parent.hovered || parent.parent.visualFocus ? Style.lightHover : "transparent"
                }
            }

            RowLayout {
                id: accountControlRowLayout
                anchors.fill: parent
                spacing: Style.userStatusSpacing
                Image {
                    id: accountAvatar
                    Layout.leftMargin: 7
                    verticalAlignment: Qt.AlignCenter
                    cache: false
                    source: model.avatar != "" ? model.avatar : "image://avatars/fallbackBlack"
                    Layout.preferredHeight: Style.accountAvatarSize
                    Layout.preferredWidth: Style.accountAvatarSize
                    Rectangle {
                        id: accountStatusIndicatorBackground
                        visible: model.isConnected &&
                                 model.serverHasUserStatus
                        width: accountStatusIndicator.sourceSize.width + 2
                        height: width
                        anchors.bottom: accountAvatar.bottom
                        anchors.right: accountAvatar.right
                        color: accountButton.hovered || accountButton.visualFocus ? "#f6f6f6" : "white"
                        radius: width*0.5
                    }
                    Image {
                        id: accountStatusIndicator
                        visible: model.isConnected &&
                                 model.serverHasUserStatus
                        source: model.statusIcon
                        cache: false
                        x: accountStatusIndicatorBackground.x + 1
                        y: accountStatusIndicatorBackground.y + 1
                        sourceSize.width: Style.accountAvatarStateIndicatorSize
                        sourceSize.height: Style.accountAvatarStateIndicatorSize

                        Accessible.role: Accessible.Indicator
                        Accessible.name: model.desktopNotificationsAllowed ? qsTr("Current user status is online") : qsTr("Current user status is do not disturb")
                    }
                }

                Column {
                    id: accountLabels
                    Layout.leftMargin: Style.accountLabelsSpacing
                    Layout.fillWidth: true
                    Layout.maximumWidth: parent.width - Style.accountLabelsSpacing
                    Label {
                        id: accountUser
                        text: name
                        elide: Text.ElideRight
                        color: "black"
                        font.pixelSize: Style.topLinePixelSize
                        font.bold: true
                        width: parent.width
                    }
                    Row {
                        visible: model.isConnected &&
                                 model.serverHasUserStatus
                        width: parent.width
                        Label {
                            id: emoji
                            visible: model.statusEmoji !== ""
                            text: statusEmoji
                            topPadding: -Style.accountLabelsSpacing
                        }
                        Label {
                            id: message
                            width: parent.width - parent.spacing - emoji.width
                            visible: model.statusMessage !== ""
                            text: statusMessage
                            elide: Text.ElideRight
                            color: "black"
                            font.pixelSize: Style.subLinePixelSize
                            leftPadding: Style.accountLabelsSpacing
                        }
                    }
                    Label {
                        id: accountServer
                        width: Style.currentAccountLabelWidth
                        height: Style.topLinePixelSize
                        text: server
                        elide: Text.ElideRight
                        color: "black"
                        font.pixelSize: Style.subLinePixelSize
                    }
                }
            }
        } // accountButton

        Button {
            id: userMoreButton
            Layout.preferredWidth: (userLineLayout.width * (1/6))
            Layout.preferredHeight: userLineLayout.height
            flat: true

            icon.source: "qrc:///client/theme/more.svg"
            icon.color: "transparent"

            Accessible.role: Accessible.ButtonMenu
            Accessible.name: qsTr("Account actions")
            Accessible.onPressAction: userMoreButtonMouseArea.clicked()

            onClicked: {
                if (userMoreButtonMenu.visible) {
                    userMoreButtonMenu.close()
                } else {
                    userMoreButtonMenu.popup()
                }
            }
            background:
                Rectangle {
                color: userMoreButton.hovered || userMoreButton.visualFocus ? "grey" : "transparent"
                opacity: 0.2
                height: userMoreButton.height - 2
                y: userMoreButton.y + 1
            }

            AutoSizingMenu {
                id: userMoreButtonMenu
                closePolicy: Menu.CloseOnPressOutsideParent | Menu.CloseOnEscape

                background: Rectangle {
                    border.color: Style.menuBorder
                    border.width: 1
                    radius: 2
                }

                MenuItem {
                    visible: model.isConnected && model.serverHasUserStatus
                    height: visible ? implicitHeight : 0
                    width: parent.width
                    text: qsTr("Set status")
                    font.pixelSize: Style.topLinePixelSize
                    hoverEnabled: true
                    onClicked: {
                        showUserStatusSelectorDialog(index)
                        accountMenu.close()
                    }
                }

                MenuItem {
                    visible: model.canLogout
                    height: visible ? implicitHeight : 0
                    width: parent.width
                    text: model.isConnected ? qsTr("Log out") : qsTr("Log in")
                    font.pixelSize: Style.topLinePixelSize
                    hoverEnabled: true
                    onClicked: {
                        model.isConnected ? UserModel.logout(index) : UserModel.login(index)
                        accountMenu.close()
                    }

                    Accessible.role: Accessible.Button
                    Accessible.name: model.isConnected ? qsTr("Log out") : qsTr("Log in")

                    onPressed: {
                        if (model.isConnected) {
                            UserModel.logout(index)
                        } else {
                            UserModel.login(index)
                        }
                        accountMenu.close()
                    }
                }

                MenuItem {
                    id: removeAccountButton
                    width: parent.width
                    text: model.removeAccountText
                    font.pixelSize: Style.topLinePixelSize
                    hoverEnabled: true
                    onClicked: {
                        UserModel.removeAccount(index)
                        accountMenu.close()
                    }

                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.onPressAction: removeAccountButton.clicked()
                }
            }
        }
    }
}   // MenuItem userLine
