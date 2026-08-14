import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    width: Math.min(300, parent.width * 0.9)
    height: Math.min(400, parent.height * 0.8)
    anchors.centerIn: parent
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var membersModel: []

    background: Rectangle {
        color: "#1a1a2e"
        radius: 12
        border.color: "#333"
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: "Room Members"
            color: "#fff"
            font.pixelSize: 18
            font.bold: true
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#333"
        }

        ListView {
            id: membersListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.membersModel
            clip: true
            spacing: 8

            delegate: Rectangle {
                width: ListView.view.width
                height: 40
                color: "transparent"
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 12

                    Rectangle {
                        width: 32
                        height: 32
                        radius: 16
                        color: modelData.role === "teacher" ? "#e94560" : "#0f3460"
                        
                        Label {
                            anchors.centerIn: parent
                            text: modelData.displayName ? modelData.displayName.charAt(0).toUpperCase() : "?"
                            color: "#fff"
                            font.pixelSize: 14
                            font.bold: true
                        }
                    }

                    Label {
                        text: modelData.displayName
                        color: "#fff"
                        font.pixelSize: 14
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    
                    Rectangle {
                        visible: modelData.role === "teacher"
                        color: "#e94560"
                        radius: 4
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 20
                        
                        Label {
                            anchors.centerIn: parent
                            text: "Teacher"
                            color: "#fff"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }
            }
        }

        Button {
            text: "Close"
            Layout.fillWidth: true
            onClicked: root.close()
            background: Rectangle { color: "#333"; radius: 6 }
            contentItem: Text {
                text: parent.text; color: "#fff"
                font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
