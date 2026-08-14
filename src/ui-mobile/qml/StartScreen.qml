import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: startScreen
    width: Math.min(600, parent.width * 0.9)
    height: Math.min(500, parent.height * 0.8)
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        color: "#1a1a2e"
        radius: 16
        border.color: "#0f3460"
        border.width: 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: "Welcome to XaoS Community"
            color: "#fff"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Item { Layout.fillHeight: true }

        Button {
            text: "Browse Public Gallery"
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            onClicked: {
                startScreen.close()
            }
            background: Rectangle {
                color: "#0f3460"
                radius: 8
            }
            contentItem: Text {
                text: parent.text; color: "#fff"
                font.pixelSize: 18; font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Button {
            text: "Join a Group (Students)"
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            onClicked: {
                startScreen.close()
                joinGroupPopup.open()
            }
            background: Rectangle {
                color: "#e94560"
                radius: 8
            }
            contentItem: Text {
                text: parent.text; color: "#fff"
                font.pixelSize: 18; font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Button {
            text: "Teacher Sign In"
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            onClicked: {
                startScreen.close()
                teacherLoginPopup.open()
            }
            background: Rectangle {
                color: "#16213e"
                radius: 8
                border.color: "#e94560"
                border.width: 2
            }
            contentItem: Text {
                text: parent.text; color: "#e94560"
                font.pixelSize: 18; font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Item { Layout.fillHeight: true }
    }
}
