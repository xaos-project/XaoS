import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * Mobile Overlay UI — sits as a transparent QQuickWidget on top of
 * the FractalWidget. All fractal rendering happens in the QWidget
 * underneath. This QML layer only handles touch gestures and UI controls.
 */
Item {
    id: root
    anchors.fill: parent

    // ─── Touch Gesture Area (full screen, behind UI controls) ───
    // Captures pinch-to-zoom, drag-to-pan, and tap-to-zoom
    MultiPointTouchArea {
        id: touchArea
        anchors.fill: parent
        z: 0  // behind UI buttons
        mouseEnabled: true
        minimumTouchPoints: 1
        maximumTouchPoints: 2
        touchPoints: [
            TouchPoint { id: tp1 },
            TouchPoint { id: tp2 }
        ]

        property bool isPinching: false
        property real initialDistance: 0.0
        property real lastDistance: 0.0

        onPressed: function(touchPoints) {
            if (touchPoints.length === 2) {
                // Two fingers: start pinch
                isPinching = true
                initialDistance = distance(tp1, tp2)
                lastDistance = initialDistance
                bridge.gesturePinchStarted()
            } else if (touchPoints.length === 1) {
                // One finger: prepare for drag or tap
                isPinching = false
                dragStartX = touchPoints[0].x
                dragStartY = touchPoints[0].y
                isDragging = false
            }
        }

        property real dragStartX: 0
        property real dragStartY: 0
        property bool isDragging: false

        onUpdated: function(touchPoints) {
            if (isPinching && tp1.pressed && tp2.pressed) {
                // Pinch gesture: zoom in/out
                var d = distance(tp1, tp2)
                var scale = d / initialDistance
                var cx = (tp1.x + tp2.x) / 2
                var cy = (tp1.y + tp2.y) / 2
                bridge.gesturePinch(scale, cx, cy)
                lastDistance = d
            } else if (touchPoints.length === 1 && !isPinching) {
                // Single finger drag: pan
                var dx = touchPoints[0].x - dragStartX
                var dy = touchPoints[0].y - dragStartY
                if (!isDragging && (Math.abs(dx) > 8 || Math.abs(dy) > 8)) {
                    isDragging = true
                }
                if (isDragging) {
                    bridge.gesturePan(dx, dy, touchPoints[0].x, touchPoints[0].y)
                }
            }
        }

        onReleased: function(touchPoints) {
            if (isPinching) {
                isPinching = false
                bridge.stopZoom()
            } else if (isDragging) {
                isDragging = false
                bridge.gesturePanFinished()
            } else {
                // Tap: zoom in briefly at the tap position
                if (touchPoints.length >= 1) {
                    bridge.startZoomIn()
                    zoomPulseTimer.restart()
                }
            }
        }

        function distance(p1, p2) {
            var dx = p2.x - p1.x
            var dy = p2.y - p1.y
            return Math.sqrt(dx*dx + dy*dy)
        }
    }

    // Timer: stops zoom after a single tap
    Timer {
        id: zoomPulseTimer
        interval: 200
        onTriggered: bridge.stopZoom()
    }

    // ─── Hamburger Menu Button ───
    RoundButton {
        id: menuButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 16
        width: 48
        height: 48
        radius: 24
        text: "☰"
        font.pixelSize: 22
        z: 10

        background: Rectangle {
            color: menuButton.pressed ? "#cc333333" : "#99222222"
            radius: menuButton.radius
            border.color: "#55ffffff"
            border.width: 1
        }

        contentItem: Text {
            text: menuButton.text
            color: "#ffffff"
            font: menuButton.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        onClicked: drawer.open()
    }

    // ─── Slide-out Drawer Menu ───
    Drawer {
        id: drawer
        width: Math.min(root.width * 0.8, 320)
        height: root.height
        edge: Qt.LeftEdge

        background: Rectangle {
            color: "#1a1a2e"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 8

            // Header
            Label {
                text: "XaoS"
                font.pixelSize: 28
                font.bold: true
                color: "#e94560"
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 8
            }

            Label {
                text: "Fractal Explorer"
                font.pixelSize: 14
                color: "#888888"
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 16
            }

            // Current formula display
            Rectangle {
                Layout.fillWidth: true
                height: 50
                radius: 8
                color: "#16213e"
                border.color: "#0f3460"

                Label {
                    anchors.centerIn: parent
                    text: bridge ? bridge.formulaName : "Mandelbrot"
                    color: "#e94560"
                    font.pixelSize: 16
                    font.bold: true
                }
            }

            // Fractal Type button
            Button {
                Layout.fillWidth: true
                text: "Change Fractal"
                onClicked: {
                    formulaPopup.open()
                    drawer.close()
                }
                background: Rectangle {
                    color: parent.pressed ? "#0f3460" : "#16213e"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 15
                }
            }

            // Iterations slider
            Label {
                text: "Iterations: " + (bridge ? bridge.maxIterations : 170)
                color: "#cccccc"
                font.pixelSize: 13
                Layout.topMargin: 8
            }
            Slider {
                id: iterSlider
                Layout.fillWidth: true
                from: 10
                to: 5000
                stepSize: 10
                value: bridge ? bridge.maxIterations : 170

                onMoved: {
                    if (bridge)
                        bridge.setIterations(Math.round(value))
                }
            }

            // Action buttons
            Button {
                Layout.fillWidth: true
                text: bridge && bridge.autopilotActive ?
                          "Stop Autopilot" : "Autopilot"
                onClicked: {
                    if (bridge) bridge.toggleAutopilot()
                }
                background: Rectangle {
                    color: bridge && bridge.autopilotActive ? "#e94560" :
                           parent.pressed ? "#0f3460" : "#16213e"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 15
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Julia Mode"
                onClicked: {
                    if (bridge) bridge.toggleJulia()
                }
                background: Rectangle {
                    color: parent.pressed ? "#0f3460" : "#16213e"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 15
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Random Palette"
                onClicked: {
                    if (bridge) bridge.randomizePalette()
                }
                background: Rectangle {
                    color: parent.pressed ? "#0f3460" : "#16213e"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 15
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Random Example"
                onClicked: {
                    if (bridge) bridge.loadRandomExample()
                    drawer.close()
                }
                background: Rectangle {
                    color: parent.pressed ? "#0f3460" : "#16213e"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 15
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Reset View"
                onClicked: {
                    if (bridge) bridge.resetView()
                    drawer.close()
                }
                background: Rectangle {
                    color: parent.pressed ? "#0f3460" : "#16213e"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 15
                }
            }

            // Spacer
            Item { Layout.fillHeight: true }

            // Undo/Redo
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    text: "↩ Undo"
                    onClicked: { if (bridge) bridge.undo() }
                    background: Rectangle {
                        color: parent.pressed ? "#0f3460" : "#16213e"
                        radius: 8
                    }
                    contentItem: Text {
                        text: parent.text; color: "#ffffff"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "↪ Redo"
                    onClicked: { if (bridge) bridge.redo() }
                    background: Rectangle {
                        color: parent.pressed ? "#0f3460" : "#16213e"
                        radius: 8
                    }
                    contentItem: Text {
                        text: parent.text; color: "#ffffff"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            // About
            Label {
                text: "XaoS v4.3 • GNU GPL v2+"
                color: "#555555"
                font.pixelSize: 11
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
            }
        }
    }

    // ─── Formula Selection Popup ───
    Popup {
        id: formulaPopup
        anchors.centerIn: parent
        width: Math.min(root.width * 0.9, 400)
        height: Math.min(root.height * 0.7, 500)
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#1a1a2e"
            radius: 12
            border.color: "#0f3460"
            border.width: 2
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 8

            Label {
                text: "Select Fractal Type"
                font.pixelSize: 20
                font.bold: true
                color: "#e94560"
                Layout.alignment: Qt.AlignHCenter
            }

            ListView {
                id: formulaList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: bridge ? bridge.formulaCount : 0
                spacing: 4

                delegate: Rectangle {
                    width: formulaList.width
                    height: 44
                    radius: 8
                    color: mouseArea2.pressed ? "#0f3460" : "#16213e"

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: bridge ? bridge.getFormulaName(index) : ""
                        color: "#ffffff"
                        font.pixelSize: 14
                    }

                    MouseArea {
                        id: mouseArea2
                        anchors.fill: parent
                        onClicked: {
                            if (bridge)
                                bridge.setFormula(index)
                            formulaPopup.close()
                        }
                    }
                }
            }
        }
    }

    // ─── Bottom Toolbar ───
    RowLayout {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 24
        spacing: 16
        z: 10

        Repeater {
            model: [
                { icon: "Auto", label: "Autopilot", action: "autopilot" },
                { icon: "Color", label: "Palette", action: "palette" },
                { icon: "Julia", label: "Julia Mode", action: "julia" },
                { icon: "Rand", label: "Random", action: "random" }
            ]

            Button {
                required property var modelData
                height: 44

                background: Rectangle {
                    color: pressed ? "#cc333333" : "#99222222"
                    radius: 22
                    border.color: "#55ffffff"
                    border.width: 1
                }

                contentItem: Text {
                    text: modelData.icon
                    font.pixelSize: 14
                    font.bold: true
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 16
                    rightPadding: 16
                }

                ToolTip.visible: hovered
                ToolTip.text: modelData.label

                onClicked: {
                    switch(modelData.action) {
                    case "autopilot":
                        if (bridge) bridge.toggleAutopilot()
                        break
                    case "palette":
                        if (bridge) bridge.randomizePalette()
                        break
                    case "julia":
                        if (bridge) bridge.toggleJulia()
                        break
                    case "random":
                        if (bridge) bridge.loadRandomExample()
                        break
                    }
                }
            }
        }
    }
}
