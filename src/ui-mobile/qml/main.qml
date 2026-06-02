import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import XaoS 1.0

ApplicationWindow {
    id: root
    visible: true
    visibility: Window.FullScreen
    title: "XaoS"
    color: "#000000"

    // Engine bridge (created in C++ main and set as context property)
    property var engine: engineBridge

    // ─── Fractal Rendering Surface ───
    FractalQuickItem {
        id: fractalView
        anchors.fill: parent
        engine: root.engine

        // ── TAP: Single tap zooms in at that point ──
        TapHandler {
            id: tapHandler
            gesturePolicy: TapHandler.ReleaseWithinBounds

            onTapped: function(eventPoint, button) {
                fractalView.startZoomIn(eventPoint.position.x,
                                        eventPoint.position.y)
                zoomPulseTimer.restart()
            }
        }

        // ── DRAG: Single finger drag to pan ──
        DragHandler {
            id: dragHandler
            target: null
            dragThreshold: 8

            onActiveChanged: {
                if (active) {
                    fractalView.stopZoom()
                    fractalView.startPan(centroid.position.x,
                                         centroid.position.y)
                } else {
                    fractalView.stopPan()
                }
            }

            onTranslationChanged: {
                if (active) {
                    fractalView.updatePan(centroid.position.x,
                                          centroid.position.y)
                }
            }
        }

        // ── PINCH: Two fingers to zoom in/out ──
        PinchHandler {
            id: pinchHandler
            target: null
            minimumPointCount: 2
            maximumPointCount: 2

            property real prevScale: 1.0

            onActiveChanged: {
                if (active) {
                    prevScale = 1.0
                    fractalView.stopPan()
                    fractalView.stopZoom()
                } else {
                    fractalView.stopZoom()
                }
            }

            onActiveScaleChanged: {
                if (!active) return

                var cx = centroid.position.x
                var cy = centroid.position.y

                var delta = activeScale / prevScale

                if (delta > 1.02) {
                    fractalView.startZoomIn(cx, cy)
                    prevScale = activeScale
                } else if (delta < 0.98) {
                    fractalView.startZoomOut(cx, cy)
                    prevScale = activeScale
                }

                fractalView.updateMousePosition(cx, cy)
            }
        }

        // Timer: stops zoom after a tap
        Timer {
            id: zoomPulseTimer
            interval: 200
            onTriggered: fractalView.stopZoom()
        }
    }

    // ─── Status Message Overlay ───
    Label {
        id: statusLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 50
        text: engine ? engine.statusMessage : ""
        color: "#ffffff"
        font.pixelSize: 14
        visible: text.length > 0
        background: Rectangle {
            color: "#80000000"
            radius: 8
        }
        padding: 8
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
                    text: engine ? engine.formulaName : "Mandelbrot"
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
                text: "Iterations: " + (engine ? engine.iterations : 170)
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
                value: engine ? engine.iterations : 170

                onMoved: {
                    if (engine)
                        engine.setIterations(Math.round(value))
                }
            }

            // Action buttons
            Button {
                Layout.fillWidth: true
                text: engine && engine.autopilotActive ?
                          "Stop Autopilot" : "Autopilot"
                onClicked: {
                    if (engine) engine.toggleAutopilot()
                }
                background: Rectangle {
                    color: engine && engine.autopilotActive ? "#e94560" :
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
                text: engine && engine.juliaMode ?
                          "Back to Mandelbrot" : "Julia Mode"
                onClicked: {
                    if (engine) engine.toggleJulia()
                }
                background: Rectangle {
                    color: engine && engine.juliaMode ? "#533483" :
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
                text: "Random Palette"
                onClicked: {
                    if (engine) engine.randomizePalette()
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
                    if (engine) engine.loadRandomExample()
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
                    if (engine) engine.resetView()
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
                    onClicked: { if (engine) engine.undo() }
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
                    onClicked: { if (engine) engine.redo() }
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
                model: engine ? engine.formulaCount : 0
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
                        text: engine ? engine.getFormulaName(index) : ""
                        color: "#ffffff"
                        font.pixelSize: 14
                    }

                    MouseArea {
                        id: mouseArea2
                        anchors.fill: parent
                        onClicked: {
                            if (engine) {
                                engine.setFormula(index)
                            }
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
                        if (engine) engine.toggleAutopilot()
                        break
                    case "palette":
                        if (engine) engine.randomizePalette()
                        break
                    case "julia":
                        if (engine) engine.toggleJulia()
                        break
                    case "random":
                        if (engine) engine.loadRandomExample()
                        break
                    }
                }
            }
        }
    }
}
