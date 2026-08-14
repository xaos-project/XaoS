import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * ShareDialog — Upload dialog for sharing the current fractal
 * to the community server. Captures title + author, auto-generates
 * thumbnail, and sends XPF data + metadata.
 */
Popup {
    id: shareDialog
    anchors.centerIn: parent
    width: parent ? Math.min(parent.width * 0.9, 380) : 380
    height: parent ? Math.min(parent.height * 0.7, 480) : 480
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property bool shareSuccess: false

    background: Rectangle {
        color: "#1a1a2e"
        radius: 16
        border.color: "#0f3460"
        border.width: 2
    }

    onOpened: {
        titleField.text = ""
        authorField.text = ""
        shareSuccess = false
        statusLabel.text = ""
        statusLabel.visible = false
    }

    Connections {
        target: community
        function onUploadComplete(id) {
            shareSuccess = true
            statusLabel.text = "Shared successfully! 🎉"
            statusLabel.color = "#4caf50"
            statusLabel.visible = true
        }
        function onNetworkError(message) {
            statusLabel.text = message
            statusLabel.color = "#ff6b6b"
            statusLabel.visible = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        Label {
            text: "Share Fractal"
            font.pixelSize: 22
            font.bold: true
            color: "#e94560"
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
                text: "Posting to:"
                color: "#00d2ff"
                font.pixelSize: 14
                font.bold: true
            }
            ComboBox {
                id: targetGroupCombo
                Layout.fillWidth: true
                textRole: "text"
                valueRole: "value"
                model: {
                    var list = [{ text: "Public Gallery", value: -1 }]
                    if (community && community.userRooms) {
                        for (var i = 0; i < community.userRooms.length; i++) {
                            list.push({ text: "Room: " + community.userRooms[i].name, value: community.userRooms[i].id })
                        }
                    }
                    return list
                }
                Component.onCompleted: updateSelection()
                onModelChanged: updateSelection()
                
                function updateSelection() {
                    if (community && community.currentGroupId > 0) {
                        for (var i = 0; i < count; i++) {
                            if (model[i].value === community.currentGroupId) {
                                currentIndex = i
                                break
                            }
                        }
                    } else {
                        currentIndex = 0
                    }
                }
                
                background: Rectangle {
                    color: "#16213e"
                    radius: 8
                    border.color: "#0f3460"
                    border.width: 1
                }
                contentItem: Text {
                    text: targetGroupCombo.currentText
                    color: "#fff"
                    font.pixelSize: 14
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                }
            }
        }

        Label {
            text: "Share your fractal creation with the XaoS community"
            font.pixelSize: 13
            color: "#888"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            text: "Title *"
            color: "#ccc"
            font.pixelSize: 13
        }
        TextField {
            id: titleField
            Layout.fillWidth: true
            placeholderText: "Give your fractal a name..."
            maximumLength: 100
            color: "#fff"
            placeholderTextColor: "#555"
            background: Rectangle {
                color: "#16213e"
                radius: 8
                border.color: titleField.activeFocus ? "#e94560" : "#0f3460"
                border.width: 1
            }
        }

        Label {
            visible: !(community && community.isLoggedIn)
            text: "Your Name (optional)"
            color: "#ccc"
            font.pixelSize: 13
        }
        TextField {
            id: authorField
            visible: !(community && community.isLoggedIn)
            Layout.fillWidth: true
            placeholderText: "Anonymous"
            maximumLength: 50
            color: "#fff"
            placeholderTextColor: "#555"
            background: Rectangle {
                color: "#16213e"
                radius: 8
                border.color: authorField.activeFocus ? "#e94560" : "#0f3460"
                border.width: 1
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 60
            radius: 8
            color: "#16213e"
            border.color: "#0f3460"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        text: bridge ? bridge.formulaName : "Mandelbrot"
                        color: "#fff"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    Label {
                        text: "Iter: " + (bridge ? bridge.maxIterations : "—") +
                              "  |  Zoom: " + (bridge ? bridge.zoomLevel : "1×")
                        color: "#888"
                        font.pixelSize: 11
                    }
                }
            }
        }

        Label {
            id: statusLabel
            visible: false
            font.pixelSize: 13
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Item { Layout.fillHeight: true }

        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            text: shareSuccess ? "Done!" :
                  (community && community.loading ? "Sharing..." : "Share")
            enabled: !shareSuccess && !(community && community.loading) &&
                     titleField.text.trim().length > 0

            onClicked: {
                if (!bridge || !community) return

                var thumbPath = bridge.getTempPath("xaos_share_thumb.png")
                bridge.saveThumbnail(thumbPath)

                var xpfData = bridge.getCurrentXpf()

                community.upload(
                    titleField.text.trim(),
                    authorField.text.trim() || "Anonymous",
                    xpfData,
                    thumbPath,
                    bridge.formulaName,
                    bridge.maxIterations,
                    bridge.zoomLevel,
                    targetGroupCombo.currentValue
                )
            }

            background: Rectangle {
                color: parent.enabled ?
                       (parent.pressed ? "#c33050" : "#e94560") :
                       (shareSuccess ? "#4caf50" : "#555")
                radius: 12
            }
            contentItem: Text {
                text: parent.text
                color: "#fff"
                font.pixelSize: 16
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Button {
            Layout.fillWidth: true
            visible: shareSuccess
            text: "Close"
            onClicked: shareDialog.close()
            background: Rectangle {
                color: parent.pressed ? "#0f3460" : "#16213e"
                radius: 12
            }
            contentItem: Text {
                text: parent.text; color: "#aaa"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
