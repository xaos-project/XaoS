import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: joinGroupPopup
    width: Math.min(600, parent.width * 0.9)
    height: Math.min(400, parent.height * 0.8)
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

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
            text: "Join a Group"
            color: "#fff"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: inviteCodeInput
            placeholderText: "6-Character Invite Code"
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

        TextField {
            id: displayNameInput
            placeholderText: "Your Display Name (e.g. Alex)"
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
                onClicked: {
                    joinGroupPopup.close()
                }
                background: Rectangle { color: "#333"; radius: 8 }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                text: "Join"
                Layout.fillWidth: true
                enabled: inviteCodeInput.text.length > 0 && displayNameInput.text.length > 0 && (!community || !community.loading)
                onClicked: {
                    if (community) {
                        community.joinGroup(inviteCodeInput.text, displayNameInput.text)
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

    Connections {
        target: community
        function onLoginSuccess() {
            if (joinGroupPopup.visible) {
                joinGroupPopup.close()
            }
        }
    }
}
