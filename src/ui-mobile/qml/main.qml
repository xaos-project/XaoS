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

        // ─── Gesture Handling ───
        // Left press+hold = zoom in at point
        // Left press+drag = pan
        // Right press = zoom out
        MouseArea {
            id: gestureArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: false

            property point pressPos: Qt.point(0, 0)
            property bool isDragging: false
            property real dragThreshold: 10  // pixels before drag becomes pan

            onPressed: function(mouse) {
                pressPos = Qt.point(mouse.x, mouse.y)
                isDragging = false

                if (mouse.button === Qt.RightButton) {
                    // Right click = zoom out
                    fractalView.startZoomOut(mouse.x, mouse.y)
                } else {
                    // Left click = start zoom in at this point
                    fractalView.startZoomIn(mouse.x, mouse.y)
                }
            }

            onPositionChanged: function(mouse) {
                var dx = mouse.x - pressPos.x
                var dy = mouse.y - pressPos.y
                var dist = Math.sqrt(dx * dx + dy * dy)

                if (!isDragging && dist > dragThreshold) {
                    // Transition from zoom to pan
                    isDragging = true
                    fractalView.stopZoom()
                    fractalView.startPan(pressPos.x, pressPos.y)
                }

                if (isDragging) {
                    // Update pan position
                    fractalView.updatePan(mouse.x, mouse.y)
                } else {
                    // Still zooming — update zoom center
                    fractalView.updateMousePosition(mouse.x, mouse.y)
                }
            }

            onReleased: {
                if (isDragging) {
                    fractalView.stopPan()
                } else {
                    fractalView.stopZoom()
                }
                isDragging = false
            }

            // Scroll wheel = zoom in/out
            onWheel: function(wheel) {
                var cx = wheel.x
                var cy = wheel.y
                if (wheel.angleDelta.y > 0) {
                    fractalView.startZoomIn(cx, cy)
                    // Brief zoom pulse
                    zoomPulseTimer.start()
                } else if (wheel.angleDelta.y < 0) {
                    fractalView.startZoomOut(cx, cy)
                    zoomPulseTimer.start()
                }
            }
        }

        // Timer to stop zoom after scroll wheel pulse
        Timer {
            id: zoomPulseTimer
            interval: 150
            onTriggered: fractalView.stopZoom()
        }

        // Pinch to zoom (for touch screens)
        PinchArea {
            anchors.fill: parent
            enabled: true
            property real lastScale: 1.0

            onPinchStarted: function(pinch) {
                lastScale = 1.0
                fractalView.updateMousePosition(pinch.center.x, pinch.center.y)
            }

            onPinchUpdated: function(pinch) {
                fractalView.updateMousePosition(pinch.center.x, pinch.center.y)
                if (pinch.scale > lastScale) {
                    fractalView.startZoomIn(pinch.center.x, pinch.center.y)
                } else if (pinch.scale < lastScale) {
                    fractalView.startZoomOut(pinch.center.x, pinch.center.y)
                }
                lastScale = pinch.scale
            }

            onPinchFinished: {
                fractalView.stopZoom()
            }
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
                text: "🔢  Change Fractal"
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
                          "🛑  Stop Autopilot" : "🚀  Autopilot"
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
                          "📐  Back to Mandelbrot" : "📐  Julia Mode"
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
                text: "🎨  Random Palette"
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
                text: "🎲  Random Example"
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
                text: "🏠  Reset View"
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
                { icon: "🚀", label: "Autopilot", action: "autopilot" },
                { icon: "🎨", label: "Palette", action: "palette" },
                { icon: "📐", label: "Julia", action: "julia" },
                { icon: "🎲", label: "Random", action: "random" }
            ]

            RoundButton {
                required property var modelData
                width: 52
                height: 52
                radius: 26

                background: Rectangle {
                    color: pressed ? "#cc333333" : "#99222222"
                    radius: parent.radius
                    border.color: "#55ffffff"
                    border.width: 1
                }

                contentItem: Text {
                    text: modelData.icon
                    font.pixelSize: 22
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
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
