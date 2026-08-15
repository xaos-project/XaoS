import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

/*
 * JoinGroup — student-facing dialog for joining a room by invite code.
 */
ThemedPopup {
    id: joinGroupPopup

    accent: Theme.accentMagenta

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
                icon: "group_add"
                size: 40
                iconColor: Theme.accentMagenta
                bgColor: Theme.alpha(Theme.accentMagenta, 0.10)
                borderColor: Theme.alpha(Theme.accentMagenta, 0.20)
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: "PRIVATE ROOM"
                    font.pixelSize: Theme.fontEyebrow
                    font.bold: true
                    font.letterSpacing: Theme.trackingWide
                    color: Theme.textDim
                }
                Text {
                    text: "Join a Group"
                    font.pixelSize: Theme.fontXl
                    font.bold: true
                    color: Theme.textPrimary
                }
            }
        }

        ThemedField {
            id: inviteCodeInput
            Layout.fillWidth: true
            label: "INVITE CODE"
            placeholderText: "6 characters"
            accent: Theme.accentMagenta
            mono: true
            uppercase: true
            maximumLength: 6
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
        }

        ThemedField {
            id: displayNameInput
            Layout.fillWidth: true
            label: "YOUR DISPLAY NAME"
            placeholderText: "e.g. Alex"
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
                onClicked: {
                    joinGroupPopup.close()
                }
            }

            PrimaryButton {
                Layout.fillWidth: true
                text: "Join"
                iconGlyph: "login"
                accent: Theme.accentMagenta
                enabled: inviteCodeInput.text.length > 0 && displayNameInput.text.length > 0 && (!community || !community.loading)
                onClicked: {
                    if (community) {
                        community.joinGroup(inviteCodeInput.text, displayNameInput.text)
                    }
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
