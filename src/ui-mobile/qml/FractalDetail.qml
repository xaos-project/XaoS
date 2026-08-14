import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * FractalDetail — Detail view shown when tapping a gallery item.
 * Displays full thumbnail, metadata, and a "Download & Explore" button.
 */
Popup {
    id: detailPopup
    anchors.centerIn: parent
    width: parent ? Math.min(parent.width * 0.9, 380) : 380
    height: parent ? Math.min(parent.height * 0.8, 560) : 560
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var fractalData: ({})

    background: Rectangle {
        color: "#1a1a2e"
        radius: 16
        border.color: "#0f3460"
        border.width: 2
    }

    Connections {
        target: community
        function onXpfDownloaded(fractalId, xpfData) {
            if (bridge) {
                bridge.loadFromXpf(xpfData)
            }
            detailPopup.close()
            if (detailPopup.parent && detailPopup.parent.close)
                detailPopup.parent.close()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            RoundButton {
                text: "\u2715"
                width: 36; height: 36
                font.pixelSize: 18
                background: Rectangle {
                    color: parent.pressed ? "#333" : "transparent"
                    radius: 18
                }
                contentItem: Text {
                    text: parent.text; color: "#aaa"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: detailPopup.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: width * 0.75
            radius: 12
            color: "#0f3460"
            clip: true

            Image {
                anchors.fill: parent
                source: fractalData.thumbnailUrl || ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true

                Label {
                    anchors.centerIn: parent
                    visible: parent.status !== Image.Ready
                    text: "No preview"
                    color: "#555"
                    font.pixelSize: 14
                }
            }
        }

        Label {
            text: fractalData.title || "Untitled"
            font.pixelSize: 22
            font.bold: true
            color: "#e94560"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            text: "by " + (fractalData.author || "Anonymous")
            font.pixelSize: 14
            color: "#aaa"
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 16
            rowSpacing: 6

            Label { text: "Formula"; color: "#888"; font.pixelSize: 12 }
            Label {
                text: fractalData.formula || "—"
                color: "#ddd"; font.pixelSize: 12
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Label { text: "Iterations"; color: "#888"; font.pixelSize: 12 }
            Label {
                text: fractalData.iterations ? fractalData.iterations.toString() : "—"
                color: "#ddd"; font.pixelSize: 12
            }

            Label { text: "Zoom"; color: "#888"; font.pixelSize: 12 }
            Label {
                text: fractalData.zoomLevel || "—"
                color: "#ddd"; font.pixelSize: 12
            }

            Label { text: "Downloads"; color: "#888"; font.pixelSize: 12 }
            Label {
                text: (fractalData.downloads || 0).toString()
                color: "#ddd"; font.pixelSize: 12
            }

            Label { text: "Likes"; color: "#888"; font.pixelSize: 12 }
            Label {
                text: (fractalData.likes || 0).toString()
                color: "#ddd"; font.pixelSize: 12
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                text: community && community.loading ? "Downloading..." : "Download & Explore"
                enabled: !(community && community.loading)

                onClicked: {
                    if (community && fractalData.id) {
                        community.downloadXpf(fractalData.id)
                    }
                }

                background: Rectangle {
                    color: parent.enabled ?
                           (parent.pressed ? "#c33050" : "#e94560") : "#555"
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
                Layout.preferredWidth: 64
                Layout.preferredHeight: 48
                text: "❤️"
                enabled: !(community && community.loading)
                onClicked: {
                    if (community && fractalData.id) {
                        community.likeFractal(fractalData.id)
                        fractalData.likes = (fractalData.likes || 0) + 1
                    }
                }
                background: Rectangle {
                    color: parent.pressed ? "#0f3460" : "#16213e"
                    radius: 12
                    border.color: "#e94560"
                    border.width: 1
                }
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 20
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
