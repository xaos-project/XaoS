import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// SettingsSheet.qml — Bottom sheet settings panel with Material Symbols icons.
// Matches the "Refined Settings" HTML mockup.

Item {
    id: root

    property bool isOpen: false

    function open() { isOpen = true; }
    function close() { isOpen = false; }

    // Dim overlay
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.5)
        opacity: root.isOpen ? 1.0 : 0.0
        visible: opacity > 0

        Behavior on opacity { NumberAnimation { duration: 250 } }

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // Sheet container
    Rectangle {
        id: sheet
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: parent.height * 0.75
        radius: 32
        color: Qt.rgba(0.102, 0.102, 0.102, 0.95)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.1)

        // Slide animation
        transform: Translate {
            y: root.isOpen ? 0 : sheet.height + 20
            Behavior on y {
                NumberAnimation { duration: 350; easing.type: Easing.OutCubic }
            }
        }

        // ─── Drag Handle ─────────────────────────
        Item {
            id: dragHandle
            anchors.top: parent.top
            width: parent.width
            height: 32

            Rectangle {
                anchors.centerIn: parent
                width: 40
                height: 5
                radius: 2.5
                color: Qt.rgba(1, 1, 1, 0.2)
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.close()
            }
        }

        // ─── Scrollable Content ──────────────────
        Flickable {
            id: contentFlickable
            anchors.top: dragHandle.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            anchors.bottomMargin: 40
            contentHeight: contentColumn.height
            clip: true
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: contentColumn
                width: parent.width
                spacing: 24

                // ─── Header ──────────────────────
                RowLayout {
                    width: parent.width

                    Text {
                        text: "Fractal Settings"
                        color: "white"
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                    }

                    // close: e5cd
                    GlassButton {
                        size: 32
                        iconCode: "\ue5cd"
                        iconSize: 18
                        onClicked: root.close()
                    }
                }

                // ─── Fractal Type ────────────────
                Column {
                    width: parent.width
                    spacing: 12

                    Text {
                        text: "FRACTAL TYPE"
                        color: Qt.rgba(1, 1, 1, 0.4)
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        font.letterSpacing: 2
                    }

                    Flickable {
                        width: parent.width
                        height: 140
                        contentWidth: fractalRow.width
                        flickableDirection: Flickable.HorizontalFlick
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds

                        Row {
                            id: fractalRow
                            spacing: 12

                            Repeater {
                                model: [
                                    { name: "Mandelbrot", selected: true, cmd: "mandel" },
                                    { name: "Julia", selected: false, cmd: "julia" },
                                    { name: "Burning Ship", selected: false, cmd: "barnsley" }
                                ]

                                delegate: Column {
                                    spacing: 8

                                    Rectangle {
                                        width: 110
                                        height: 110
                                        radius: 16
                                        color: Qt.rgba(1, 1, 1, 0.05)
                                        border.width: modelData.selected ? 2 : 1
                                        border.color: modelData.selected ? "#E13535" : Qt.rgba(1, 1, 1, 0.1)

                                        Rectangle {
                                            anchors.fill: parent
                                            anchors.margins: 2
                                            radius: 14
                                            gradient: Gradient {
                                                GradientStop { position: 0.0; color: modelData.selected ? "#2D1B4E" : "#1A1A2E" }
                                                GradientStop { position: 0.5; color: modelData.selected ? "#4A1942" : "#16213E" }
                                                GradientStop { position: 1.0; color: modelData.selected ? "#1A1A2E" : "#0F3460" }
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: bridge.executeCommand(modelData.cmd)
                                        }
                                    }

                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: modelData.name
                                        color: modelData.selected ? "white" : Qt.rgba(1, 1, 1, 0.4)
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }
                        }
                    }
                }

                // ─── Color Palette ───────────────
                Column {
                    width: parent.width
                    spacing: 12

                    Text {
                        text: "COLOR PALETTE"
                        color: Qt.rgba(1, 1, 1, 0.4)
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        font.letterSpacing: 2
                    }

                    Row {
                        spacing: 12

                        Repeater {
                            model: [
                                { from: "#DC2626", to: "#F87171", selected: true },
                                { from: "#2563EB", to: "#22D3EE", selected: false },
                                { from: "#D97706", to: "#FB923C", selected: false },
                                { from: "#3F3F46", to: "#A1A1AA", selected: false }
                            ]

                            delegate: Rectangle {
                                width: 80
                                height: 40
                                radius: 8
                                opacity: modelData.selected ? 1.0 : 0.6
                                border.width: modelData.selected ? 2 : 0
                                border.color: modelData.selected ? Qt.rgba(0.882, 0.208, 0.208, 0.4) : "transparent"

                                gradient: Gradient {
                                    orientation: Gradient.Horizontal
                                    GradientStop { position: 0.0; color: modelData.from }
                                    GradientStop { position: 1.0; color: modelData.to }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: bridge.executeCommand("defaultpalette")
                                }
                            }
                        }
                    }
                }

                // ─── Zoom Speed Slider ───────────
                Column {
                    width: parent.width
                    spacing: 8

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Zoom Speed"
                            color: Qt.rgba(1, 1, 1, 0.8)
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            Layout.fillWidth: true
                        }
                        Text {
                            text: (zoomSlider.value / 10).toFixed(1) + "x"
                            color: "#E13535"
                            font.pixelSize: 14
                            font.weight: Font.Bold
                        }
                    }

                    Slider {
                        id: zoomSlider
                        width: parent.width
                        from: 5
                        to: 50
                        value: 25
                        stepSize: 1

                        background: Rectangle {
                            x: zoomSlider.leftPadding
                            y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                            width: zoomSlider.availableWidth
                            height: 6
                            radius: 3
                            color: Qt.rgba(1, 1, 1, 0.1)

                            Rectangle {
                                width: zoomSlider.visualPosition * parent.width
                                height: parent.height
                                radius: 3
                                color: "#E13535"
                            }
                        }

                        handle: Rectangle {
                            x: zoomSlider.leftPadding + zoomSlider.visualPosition * (zoomSlider.availableWidth - width)
                            y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                            width: 18
                            height: 18
                            radius: 9
                            color: "white"

                            Rectangle {
                                anchors.centerIn: parent
                                width: 26
                                height: 26
                                radius: 13
                                color: Qt.rgba(0.882, 0.208, 0.208, 0.3)
                                visible: zoomSlider.pressed
                            }
                        }
                    }
                }

                // ─── Iteration Depth Slider ──────
                Column {
                    width: parent.width
                    spacing: 8

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Iteration Depth"
                            color: Qt.rgba(1, 1, 1, 0.8)
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            Layout.fillWidth: true
                        }
                        Text {
                            text: Math.round(iterSlider.value).toString()
                            color: "#E13535"
                            font.pixelSize: 14
                            font.weight: Font.Bold
                        }
                    }

                    Slider {
                        id: iterSlider
                        width: parent.width
                        from: 64
                        to: 4096
                        value: bridge.maxIterations > 0 ? bridge.maxIterations : 1024
                        stepSize: 64

                        background: Rectangle {
                            x: iterSlider.leftPadding
                            y: iterSlider.topPadding + iterSlider.availableHeight / 2 - height / 2
                            width: iterSlider.availableWidth
                            height: 6
                            radius: 3
                            color: Qt.rgba(1, 1, 1, 0.1)

                            Rectangle {
                                width: iterSlider.visualPosition * parent.width
                                height: parent.height
                                radius: 3
                                color: "#E13535"
                            }
                        }

                        handle: Rectangle {
                            x: iterSlider.leftPadding + iterSlider.visualPosition * (iterSlider.availableWidth - width)
                            y: iterSlider.topPadding + iterSlider.availableHeight / 2 - height / 2
                            width: 18
                            height: 18
                            radius: 9
                            color: "white"

                            Rectangle {
                                anchors.centerIn: parent
                                width: 26
                                height: 26
                                radius: 13
                                color: Qt.rgba(0.882, 0.208, 0.208, 0.3)
                                visible: iterSlider.pressed
                            }
                        }
                    }
                }

                // ─── Performance Mode Toggle ─────
                Rectangle {
                    width: parent.width
                    height: 64
                    radius: 16
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.width: 1
                    border.color: Qt.rgba(1, 1, 1, 0.05)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        Column {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: "Performance Mode"
                                color: "white"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: "Optimized rendering"
                                color: Qt.rgba(1, 1, 1, 0.4)
                                font.pixelSize: 12
                            }
                        }

                        Switch {
                            id: perfSwitch
                            checked: true

                            indicator: Rectangle {
                                width: 44
                                height: 24
                                radius: 12
                                color: perfSwitch.checked ? "#E13535" : Qt.rgba(1, 1, 1, 0.2)

                                Behavior on color { ColorAnimation { duration: 150 } }

                                Rectangle {
                                    x: perfSwitch.checked ? parent.width - width - 4 : 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 16
                                    height: 16
                                    radius: 8
                                    color: "white"

                                    Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.InOutQuad } }
                                }
                            }

                            contentItem: Item {}
                        }
                    }
                }

                // ─── Action Buttons ──────────────
                Column {
                    width: parent.width
                    spacing: 12

                    Rectangle {
                        width: parent.width
                        height: 52
                        radius: 16
                        color: "#E13535"

                        Text {
                            anchors.centerIn: parent
                            text: "Apply View"
                            color: "white"
                            font.pixelSize: 14
                            font.weight: Font.Bold
                        }

                        MouseArea {
                            anchors.fill: parent
                            onPressed: parent.scale = 0.98
                            onReleased: { parent.scale = 1.0; root.close(); }
                            onCanceled: parent.scale = 1.0
                        }

                        Behavior on scale { NumberAnimation { duration: 100 } }
                    }

                    Rectangle {
                        width: parent.width
                        height: 52
                        radius: 16
                        color: "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "Reset Default"
                            color: Qt.rgba(1, 1, 1, 0.4)
                            font.pixelSize: 14
                            font.weight: Font.Bold
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                bridge.executeCommand("initstate");
                                root.close();
                            }
                        }
                    }
                }

                Item { width: 1; height: 20 }
            }
        }
    }
}
