import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

/*
 * CreateRoom — teacher-facing dialog for creating a room, followed by a
 * confirmation showing the generated invite code.
 */
ThemedPopup {
    id: createRoomPopup

    accent: Theme.accentMagenta

    property string createdInviteCode: ""

    onOpened: {
        roomNameInput.text = ""
        createdInviteCode = ""
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

    contentItem: ColumnLayout {
        spacing: Theme.s4

        Row {
            Layout.fillWidth: true
            spacing: Theme.s3

            IconBadge {
                anchors.verticalCenter: parent.verticalCenter
                icon: "add_business"
                size: 40
                iconColor: Theme.accentMagenta
                bgColor: Theme.alpha(Theme.accentMagenta, 0.10)
                borderColor: Theme.alpha(Theme.accentMagenta, 0.20)
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: "CLASSROOM"
                    font.pixelSize: Theme.fontEyebrow
                    font.bold: true
                    font.letterSpacing: Theme.trackingWide
                    color: Theme.textDim
                }
                Text {
                    text: "Create a New Room"
                    font.pixelSize: Theme.fontXl
                    font.bold: true
                    color: Theme.textPrimary
                }
            }
        }

        ThemedField {
            id: roomNameInput
            Layout.fillWidth: true
            label: "ROOM NAME"
            placeholderText: "e.g. Math Class 101"
            accent: Theme.accentMagenta
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: errorText.implicitHeight + Theme.s3
            visible: community ? !!community.errorMessage : false
            radius: Theme.radiusSm
            color: Theme.alpha(Theme.danger, 0.10)
            border.color: Theme.alpha(Theme.danger, 0.30)
            border.width: 1

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Theme.s2
                anchors.rightMargin: Theme.s2
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.s2

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "error_outline"
                    font.family: Theme.iconFont
                    font.pixelSize: Theme.fontLg
                    color: Theme.danger
                }
                Text {
                    id: errorText
                    width: parent.width - Theme.fontLg - Theme.s2
                    anchors.verticalCenter: parent.verticalCenter
                    text: community ? community.errorMessage : ""
                    color: Theme.danger
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.Wrap
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
            spacing: Theme.s3

            GhostButton {
                Layout.fillWidth: true
                text: "Cancel"
                accent: Theme.textSecondary
                onClicked: createRoomPopup.close()
            }

            PrimaryButton {
                Layout.fillWidth: true
                text: "Create"
                iconGlyph: "add"
                accent: Theme.accentMagenta
                enabled: roomNameInput.text.length > 0 && (!community || !community.loading)
                onClicked: {
                    if (community) {
                        community.createRoom(roomNameInput.text)
                    }
                }
            }
        }
    }

    ThemedPopup {
        id: successDialog
        parent: createRoomPopup.parent
        accent: Theme.accentGreen
        maxWidth: 380

        contentItem: ColumnLayout {
            spacing: Theme.s4

            Column {
                Layout.fillWidth: true
                spacing: Theme.s2

                IconBadge {
                    anchors.horizontalCenter: parent.horizontalCenter
                    icon: "celebration"
                    size: 48
                    iconColor: Theme.accentGreen
                    bgColor: Theme.alpha(Theme.accentGreen, 0.10)
                    borderColor: Theme.alpha(Theme.accentGreen, 0.20)
                }

                Text {
                    width: parent.width
                    text: "Room Created"
                    font.pixelSize: Theme.fontXl
                    font.bold: true
                    color: Theme.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 92
                radius: Theme.radiusLg
                color: Theme.bgCard
                border.color: Theme.alpha(Theme.accentMagenta, 0.30)
                border.width: 1

                Column {
                    anchors.centerIn: parent
                    spacing: Theme.s1

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "INVITE CODE"
                        color: Theme.textDim
                        font.pixelSize: Theme.fontEyebrow
                        font.bold: true
                        font.letterSpacing: Theme.trackingWide
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: createRoomPopup.createdInviteCode
                        color: Theme.accentMagenta
                        font.pixelSize: 34
                        font.family: "monospace"
                        font.bold: true
                        font.letterSpacing: 6
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: "Share this code with your students."
                color: Theme.textSecondary
                font.pixelSize: Theme.fontBody
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            PrimaryButton {
                Layout.fillWidth: true
                text: "Got it"
                iconGlyph: "check"
                accent: Theme.accentGreen
                onClicked: {
                    successDialog.close()
                    createRoomPopup.close()
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
