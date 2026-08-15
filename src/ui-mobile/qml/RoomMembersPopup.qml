import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import "."

/*
 * RoomMembersPopup — roster for the currently selected room.
 */
ThemedPopup {
    id: root

    accent: Theme.accentPurple
    maxWidth: 360

    property var membersModel: []

    contentItem: ColumnLayout {
        spacing: Theme.s3

        Row {
            Layout.fillWidth: true
            spacing: Theme.s3

            IconBadge {
                anchors.verticalCenter: parent.verticalCenter
                icon: "group"
                size: 38
                iconColor: Theme.accentPurple
                bgColor: Theme.alpha(Theme.accentPurple, 0.10)
                borderColor: Theme.alpha(Theme.accentPurple, 0.20)
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: "ROSTER"
                    font.pixelSize: Theme.fontEyebrow
                    font.bold: true
                    font.letterSpacing: Theme.trackingWide
                    color: Theme.textDim
                }
                Text {
                    text: "Room Members"
                    font.pixelSize: Theme.fontLg
                    font.bold: true
                    color: Theme.textPrimary
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderSubtle
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s3
            Layout.bottomMargin: Theme.s3
            visible: root.membersModel.length === 0
            text: "No members yet."
            color: Theme.textDim
            font.pixelSize: Theme.fontBody
            horizontalAlignment: Text.AlignHCenter
        }

        ListView {
            id: membersListView
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 320)
            visible: root.membersModel.length > 0
            model: root.membersModel
            clip: true
            spacing: Theme.s1

            ScrollBar.vertical: Basic.ScrollBar {
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 3
                    radius: 1.5
                    color: Theme.alpha(Theme.accentPurple, 0.25)
                }
            }

            delegate: Rectangle {
                width: ListView.view.width
                height: 48
                radius: Theme.radiusMd
                color: Theme.bgCard
                border.color: Theme.borderSubtle
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.s2
                    anchors.rightMargin: Theme.s2
                    spacing: Theme.s3

                    Rectangle {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        radius: 16
                        color: modelData.role === "teacher"
                               ? Theme.alpha(Theme.accentMagenta, 0.16)
                               : Theme.alpha(Theme.accentCyan, 0.12)
                        border.width: 1
                        border.color: modelData.role === "teacher"
                                      ? Theme.alpha(Theme.accentMagenta, 0.40)
                                      : Theme.alpha(Theme.accentCyan, 0.30)

                        Text {
                            anchors.centerIn: parent
                            text: modelData.displayName
                                  ? modelData.displayName.charAt(0).toUpperCase() : "?"
                            color: modelData.role === "teacher"
                                   ? Theme.accentMagenta : Theme.accentCyan
                            font.pixelSize: Theme.fontMd
                            font.bold: true
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: modelData.displayName
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontMd
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        visible: modelData.role === "teacher"
                        Layout.preferredWidth: roleLabel.implicitWidth + Theme.s3
                        Layout.preferredHeight: 20
                        radius: 10
                        color: Theme.alpha(Theme.accentMagenta, 0.14)
                        border.color: Theme.alpha(Theme.accentMagenta, 0.32)
                        border.width: 1

                        Text {
                            id: roleLabel
                            anchors.centerIn: parent
                            text: "TEACHER"
                            color: Theme.accentMagenta
                            font.pixelSize: Theme.fontEyebrow
                            font.bold: true
                            font.letterSpacing: 1
                        }
                    }
                }
            }
        }

        GhostButton {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
            text: "Close"
            accent: Theme.textSecondary
            onClicked: root.close()
        }
    }
}
