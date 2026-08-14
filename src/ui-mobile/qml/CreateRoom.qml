import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: createRoomPopup
    width: Math.min(600, parent.width * 0.9)
    height: Math.min(300, parent.height * 0.6)
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string createdInviteCode: ""

    onOpened: {
        roomNameInput.text = ""
        createdInviteCode = ""
    }

    background: Rectangle {
        color: "#1a1a2e"
        radius: 16
        border.color: "#0f3460"
        border.width: 2
    }

    Timer {
        id: errorClearTimer
        interval: 3500
        repeat: false
        onTriggered: {
            if (community) community.clearError()
        }
    }

    Connections {
        target: community
        function onErrorChanged() {
            if (community && community.errorMessage !== "") {
                errorClearTimer.restart()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: "Create a New Room"
            color: "#fff"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: roomNameInput
            placeholderText: "Room Name (e.g. Math Class 101)"
            Layout.fillWidth: true
            font.pixelSize: 18
            color: "#000"
            placeholderTextColor: "#666"
            background: Rectangle {
                color: "#fff"
                radius: 8
                border.color: "#ccc"
                border.width: 1
            }
        }

        Label {
            visible: community ? !!community.errorMessage : false
            text: community ? community.errorMessage : ""
            color: "#ff6b6b"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Button {
                text: "Cancel"
                Layout.fillWidth: true
                onClicked: createRoomPopup.close()
                background: Rectangle { color: "#333"; radius: 8 }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                text: "Create"
                Layout.fillWidth: true
                enabled: roomNameInput.text.length > 0 && (!community || !community.loading)
                onClicked: {
                    if (community) {
                        community.createRoom(roomNameInput.text)
                    }
                }
                background: Rectangle {
                    color: parent.enabled ? "#e94560" : "#555"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    Popup {
        id: successDialog
        x: Math.round((createRoomPopup.width - width) / 2)
        y: Math.round((createRoomPopup.height - height) / 2)
        width: Math.min(450, createRoomPopup.width * 0.95)
        height: 280
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: "#1a1a2e"
            radius: 16
            border.color: "#e94560"
            border.width: 2
            
            Rectangle {
                anchors.fill: parent
                anchors.margins: -1
                color: "transparent"
                border.color: "#e94560"
                border.width: 4
                opacity: 0.2
                radius: 17
            }
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 12
            
            Label {
                text: "🎉 Room Created!"
                color: "#fff"
                font.pixelSize: 22
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Item { Layout.fillHeight: true }
            
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                color: "#16213e"
                radius: 12
                border.color: "#0f3460"
                border.width: 1
                
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    Label {
                        text: "INVITE CODE"
                        color: "#888"
                        font.pixelSize: 12
                        font.bold: true
                        font.letterSpacing: 2
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Label {
                        text: createRoomPopup.createdInviteCode
                        color: "#e94560"
                        font.pixelSize: 36
                        font.bold: true
                        font.letterSpacing: 4
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            Label {
                text: "Share this code with your students."
                color: "#aaa"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 8
            }
            
            Button {
                text: "Got it"
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                onClicked: {
                    successDialog.close()
                    createRoomPopup.close()
                }
                background: Rectangle {
                    color: parent.pressed ? "#111" : "#e94560"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.pixelSize: 16; font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    Connections {
        target: community
        function onRoomCreated(id, name, inviteCode) {
            if (createRoomPopup.visible) {
                createRoomPopup.createdInviteCode = inviteCode
                successDialog.open()
            }
        }
    }
}
