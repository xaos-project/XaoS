import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * XaoS Mobile Overlay UI
 */
Item {
    id: root
    anchors.fill: parent

    // ── Design tokens ──
    readonly property color bgDark:    "#0a0e1a"
    readonly property color bgCard:    "#111827"
    readonly property color bgSurface: "#1a2035"
    readonly property color accentCyan:    "#00d2ff"
    readonly property color accentMagenta: "#e94560"
    readonly property color accentPurple:  "#6c63ff"
    readonly property color textPrimary:   "#e8eaf0"
    readonly property color textSecondary: "#8892a4"
    readonly property color textDim:       "#4a5568"
    readonly property color borderSubtle:  "#1e293b"

    FontLoader { id: materialFont; source: "qrc:/fonts/MaterialIcons-Regular.ttf" }

    // ═══════════════════════════════════════════════════════════
    // TOUCH GESTURE AREA — full-screen, behind all UI
    // ═══════════════════════════════════════════════════════════
    MultiPointTouchArea {
        id: touchArea
        anchors.fill: parent
        z: 0
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
        property real dragStartX: 0
        property real dragStartY: 0
        property bool isDragging: false

        onPressed: function(touchPoints) {
            if (touchPoints.length === 2) {
                isPinching = true
                initialDistance = distance(tp1, tp2)
                lastDistance = initialDistance
                bridge.gesturePinchStarted()
            } else if (touchPoints.length === 1) {
                isPinching = false
                dragStartX = touchPoints[0].x
                dragStartY = touchPoints[0].y
                isDragging = false
            }
        }

        onUpdated: function(touchPoints) {
            if (isPinching && tp1.pressed && tp2.pressed) {
                var d = distance(tp1, tp2)
                var scale = d / initialDistance
                var cx = (tp1.x + tp2.x) / 2
                var cy = (tp1.y + tp2.y) / 2
                bridge.gesturePinch(scale, cx, cy)
                lastDistance = d
            } else if (touchPoints.length === 1 && !isPinching) {
                var dx = touchPoints[0].x - dragStartX
                var dy = touchPoints[0].y - dragStartY
                if (!isDragging && (Math.abs(dx) > 8 || Math.abs(dy) > 8))
                    isDragging = true
                if (isDragging)
                    bridge.gesturePan(dx, dy, touchPoints[0].x, touchPoints[0].y)
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

    Timer {
        id: zoomPulseTimer
        interval: 200
        onTriggered: bridge.stopZoom()
    }

    // ═══════════════════════════════════════════════════════════
    // TOP BAR — Menu icon (left) + XAOS wordmark (right)
    // ═══════════════════════════════════════════════════════════
    Item {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56
        z: 20

        // Menu button — crystalline icon
        Rectangle {
            id: menuBtn
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            width: 40; height: 40; radius: 12
            color: menuBtnArea.pressed ? Qt.rgba(0, 0.82, 1, 0.15) : "transparent"
            border.color: menuBtnArea.pressed ? accentCyan : "transparent"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "menu"
                font.family: materialFont.name
                font.pixelSize: 24
                color: accentCyan
            }

            MouseArea {
                id: menuBtnArea
                anchors.fill: parent
                onClicked: drawer.open()
            }

            Behavior on color { ColorAnimation { duration: 150 } }
        }

        // XAOS wordmark — top right
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            text: "XAOS"
            font.pixelSize: 16
            font.bold: true
            font.letterSpacing: 4
            color: textPrimary
        }
    }

    // ═══════════════════════════════════════════════════════════
    // STATS OVERLAY — Left side, showing fractal info
    // ═══════════════════════════════════════════════════════════
    Column {
        id: statsOverlay
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.topMargin: 4
        spacing: 2
        z: 15
        visible: bridge !== null

        // Formula name
        Row {
            spacing: 8
            Text {
                text: "FRAC"
                font.pixelSize: 10
                font.bold: true
                font.letterSpacing: 2
                color: accentCyan
            }
            Text {
                text: bridge ? bridge.formulaName : "—"
                font.pixelSize: 10
                font.family: "monospace"
                color: textPrimary
            }
        }

        // Iterations
        Row {
            spacing: 8
            Text {
                text: "ITER"
                font.pixelSize: 10
                font.bold: true
                font.letterSpacing: 2
                color: accentCyan
            }
            Text {
                text: bridge ? bridge.maxIterations.toString() : "—"
                font.pixelSize: 10
                font.family: "monospace"
                color: textPrimary
            }
        }
    }

    // ═══════════════════════════════════════════════════════════
    // RIGHT-SIDE ZOOM CONTROLS — Minimal floating buttons
    // ═══════════════════════════════════════════════════════════
    Column {
        id: zoomControls
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 14
        spacing: 10
        z: 15

        ZoomFab {
            icon: "add"
            holdable: true
            onAction: bridge.startZoomIn()
            onRelease: bridge.stopZoom()
        }
        ZoomFab {
            icon: "remove"
            holdable: true
            onAction: bridge.startZoomOut()
            onRelease: bridge.stopZoom()
        }
        ZoomFab {
            icon: "restart_alt"
            holdable: false
            onAction: { if (bridge) bridge.resetView() }
        }
    }

    component ZoomFab: Rectangle {
        property string icon: ""
        property bool holdable: false
        signal action()
        signal release()

        width: 42; height: 42; radius: 14
        color: fabArea.pressed ? Qt.rgba(0, 0.82, 1, 0.12) : Qt.rgba(0.04, 0.06, 0.1, 0.65)
        border.color: fabArea.pressed ? accentCyan : borderSubtle
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: parent.icon
            font.family: materialFont.name
            font.pixelSize: 24
            color: fabArea.pressed ? accentCyan : textPrimary
        }

        MouseArea {
            id: fabArea
            anchors.fill: parent
            onPressed: parent.action()
            onReleased: { if (parent.holdable) parent.release() }
            onClicked: { if (!parent.holdable) parent.action() }
        }

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    // ═══════════════════════════════════════════════════════════
    // BOTTOM TAB BAR — 5-icon navigation bar
    // ═══════════════════════════════════════════════════════════
    Rectangle {
        id: bottomTabBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 58
        z: 20
        color: Qt.rgba(0.04, 0.055, 0.1, 0.92)
        border.color: borderSubtle
        border.width: 1

        // Top edge glow line
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.2; color: Qt.rgba(0, 0.82, 1, 0.3) }
                GradientStop { position: 0.5; color: Qt.rgba(0, 0.82, 1, 0.5) }
                GradientStop { position: 0.8; color: Qt.rgba(0, 0.82, 1, 0.3) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Row {
            anchors.centerIn: parent
            spacing: 0

            // Formulas
            TabBarIcon {
                icon: "functions"; label: "Formulas"
                onTapped: { formulaPopup.open() }
            }

            // Autopilot
            TabBarIcon {
                icon: "play_circle"; label: "Autopilot"
                isActive: bridge ? bridge.autopilotActive : false
                onTapped: { if (bridge) bridge.toggleAutopilot() }
            }

            // Palette
            TabBarIcon {
                icon: "palette"; label: "Palette"
                onTapped: { if (bridge) bridge.randomizePalette() }
            }

            // Julia
            TabBarIcon {
                icon: "blur_on"; label: "Julia"
                onTapped: { if (bridge) bridge.toggleJulia() }
            }

            // Settings (opens drawer)
            TabBarIcon {
                icon: "settings"; label: "Settings"
                onTapped: { drawer.open() }
            }
        }
    }

    component TabBarIcon: Item {
        property string icon: ""
        property string label: ""
        property bool isActive: false
        signal tapped()

        width: root.width / 5
        height: 58

        Column {
            anchors.centerIn: parent
            spacing: 3

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: parent.parent.icon
                font.family: materialFont.name
                font.pixelSize: 24
                color: parent.parent.isActive ? accentCyan : textSecondary
                Behavior on color { ColorAnimation { duration: 200 } }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: parent.parent.label
                font.pixelSize: 9
                font.letterSpacing: 0.5
                color: parent.parent.isActive ? accentCyan : textDim
                Behavior on color { ColorAnimation { duration: 200 } }
            }
        }

        // Active indicator dot
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 2
            width: 4; height: 4; radius: 2
            color: accentCyan
            visible: parent.isActive
        }

        MouseArea {
            anchors.fill: parent
            onClicked: parent.tapped()
        }
    }

    // ═══════════════════════════════════════════════════════════
    // DRAWER — Settings & Controls Panel
    // ═══════════════════════════════════════════════════════════
    Drawer {
        id: drawer
        width: Math.min(root.width * 0.85, 360)
        height: root.height
        edge: Qt.LeftEdge
        dim: true

        background: Rectangle {
            color: bgDark

            // Right-edge accent line
            Rectangle {
                width: 1; height: parent.height
                anchors.right: parent.right
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 0.3; color: accentCyan }
                    GradientStop { position: 0.7; color: accentMagenta }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }
        }

        Flickable {
            anchors.fill: parent
            contentHeight: drawerCol.height + 40
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: drawerCol
                width: parent.width
                spacing: 0

                // ── Header ──
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100

                    // Top accent bar
                    Rectangle {
                        width: parent.width; height: 2
                        anchors.top: parent.top
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: accentCyan }
                            GradientStop { position: 0.5; color: accentPurple }
                            GradientStop { position: 1.0; color: accentMagenta }
                        }
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 6

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "XAOS"
                            font.pixelSize: 28
                            font.bold: true
                            font.letterSpacing: 6
                            color: textPrimary
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "FRACTAL EXPLORER"
                            font.pixelSize: 10
                            font.letterSpacing: 4
                            font.weight: Font.Medium
                            color: accentCyan
                        }
                    }
                }

                // ── Current Fractal Card ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    height: 64; radius: 12
                    color: bgCard
                    border.color: borderSubtle; border.width: 1

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 14; anchors.rightMargin: 14
                        spacing: 12

                        // Icon badge
                        Rectangle {
                            width: 38; height: 38
                            anchors.verticalCenter: parent.verticalCenter
                            radius: 10
                            color: Qt.rgba(0, 0.82, 1, 0.08)
                            border.color: Qt.rgba(0, 0.82, 1, 0.2)
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "all_inclusive"
                                font.family: materialFont.name
                                font.pixelSize: 24
                                color: accentCyan
                            }
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2
                            Text {
                                text: "CURRENT FRACTAL"
                                font.pixelSize: 9; font.letterSpacing: 2
                                font.weight: Font.Medium
                                color: textDim
                            }
                            Text {
                                text: bridge ? bridge.formulaName : "Mandelbrot"
                                font.pixelSize: 15; font.bold: true
                                color: accentCyan
                            }
                        }
                    }
                }

                // ── Rendering & Compute Section ──
                SectionHeader { text: "RENDERING & COMPUTE" }

                // Iteration Depth card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    height: 90; radius: 12
                    color: bgCard
                    border.color: borderSubtle; border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        Row {
                            width: parent.width
                            Text {
                                text: "Iteration Depth"
                                font.pixelSize: 13; font.weight: Font.Medium
                                color: textPrimary
                            }
                            Item { width: 1; height: 1; Layout.fillWidth: true }
                            Text {
                                anchors.right: parent.right
                                text: bridge ? bridge.maxIterations.toString() : "170"
                                font.pixelSize: 13; font.bold: true
                                font.family: "monospace"
                                color: accentMagenta
                            }
                        }

                        Slider {
                            id: iterSlider
                            width: parent.width
                            from: 10; to: 5000; stepSize: 10
                            value: bridge ? bridge.maxIterations : 170

                            background: Rectangle {
                                x: iterSlider.leftPadding
                                y: iterSlider.topPadding + iterSlider.availableHeight / 2 - height / 2
                                width: iterSlider.availableWidth; height: 3; radius: 1.5
                                color: Qt.rgba(1,1,1,0.08)

                                Rectangle {
                                    width: iterSlider.visualPosition * parent.width
                                    height: parent.height; radius: 1.5
                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop { position: 0.0; color: accentCyan }
                                        GradientStop { position: 1.0; color: accentPurple }
                                    }
                                }
                            }

                            handle: Rectangle {
                                x: iterSlider.leftPadding + iterSlider.visualPosition * (iterSlider.availableWidth - width)
                                y: iterSlider.topPadding + iterSlider.availableHeight / 2 - height / 2
                                width: 18; height: 18; radius: 9
                                color: iterSlider.pressed ? accentCyan : "#ffffff"
                                border.color: accentCyan; border.width: 2

                                Behavior on color { ColorAnimation { duration: 100 } }
                            }

                            onMoved: {
                                if (bridge) bridge.setIterations(Math.round(value))
                            }
                        }
                    }
                }

                // ── Controls Section ──
                SectionHeader { text: "CONTROLS" }

                DrawerItem {
                    icon: "play_circle"; label: "Autopilot"
                    subtitle: bridge && bridge.autopilotActive ? "Active" : "Off"
                    isActive: bridge ? bridge.autopilotActive : false
                    Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                    onTapped: { if (bridge) bridge.toggleAutopilot() }
                }

                DrawerItem {
                    icon: "blur_on"; label: "Julia Mode"
                    subtitle: "Toggle Julia set"
                    Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                    onTapped: { if (bridge) bridge.toggleJulia() }
                }

                DrawerItem {
                    icon: "palette"; label: "Random Palette"
                    subtitle: "Generate new colors"
                    Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                    onTapped: { if (bridge) bridge.randomizePalette() }
                }

                DrawerItem {
                    icon: "casino"; label: "Random Example"
                    subtitle: "Load a preset fractal"
                    Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                    onTapped: { if (bridge) bridge.loadRandomExample(); drawer.close() }
                }

                DrawerItem {
                    icon: "restart_alt"; label: "Reset View"
                    subtitle: "Return to default"
                    Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                    onTapped: { if (bridge) bridge.resetView(); drawer.close() }
                }

                // ── History Section ──
                SectionHeader { text: "HISTORY" }

                Row {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    spacing: 8

                    DrawerItem {
                        icon: "undo"; label: "Undo"
                        width: (parent.width - 8) / 2
                        compact: true
                        onTapped: { if (bridge) bridge.undo() }
                    }
                    DrawerItem {
                        icon: "redo"; label: "Redo"
                        width: (parent.width - 8) / 2
                        compact: true
                        onTapped: { if (bridge) bridge.redo() }
                    }
                }

                // ── Footer ──
                Item { Layout.preferredHeight: 24 }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "XaoS v4.3  ·  GNU GPL v2+"
                    font.pixelSize: 10; font.letterSpacing: 1
                    color: textDim
                }

                Item { Layout.preferredHeight: 30 }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════
    // SECTION HEADER component
    // ═══════════════════════════════════════════════════════════
    component SectionHeader: Item {
        property string text: ""
        Layout.fillWidth: true
        Layout.preferredHeight: 40
        Layout.leftMargin: 16; Layout.rightMargin: 16
        Layout.topMargin: 12

        Text {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            text: parent.text
            font.pixelSize: 10; font.letterSpacing: 3
            font.weight: Font.DemiBold
            color: accentCyan
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width; height: 1
            color: borderSubtle
        }
    }

    // ═══════════════════════════════════════════════════════════
    // DRAWER ITEM component — icon + label + optional subtitle
    // ═══════════════════════════════════════════════════════════
    component DrawerItem: Rectangle {
        id: di
        property string icon: ""
        property string label: ""
        property string subtitle: ""
        property bool isActive: false
        property bool compact: false
        signal tapped()

        height: compact ? 46 : 56
        radius: 12
        color: diArea.pressed
               ? Qt.rgba(0, 0.82, 1, 0.08)
               : isActive
                 ? Qt.rgba(0, 0.82, 1, 0.06)
                 : bgCard
        border.color: isActive ? Qt.rgba(0, 0.82, 1, 0.25) : borderSubtle
        border.width: 1
        Layout.topMargin: 4

        Row {
            anchors.fill: parent
            anchors.leftMargin: 14; anchors.rightMargin: 14
            spacing: 12

            // Icon in subtle box
            Rectangle {
                width: 32; height: 32
                anchors.verticalCenter: parent.verticalCenter
                radius: 8
                color: di.isActive ? Qt.rgba(0, 0.82, 1, 0.1) : Qt.rgba(1,1,1,0.04)
                border.color: di.isActive ? Qt.rgba(0, 0.82, 1, 0.2) : "transparent"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: di.icon
                    font.family: materialFont.name
                    font.pixelSize: 20
                    color: di.isActive ? accentCyan : textSecondary
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1
                Text {
                    text: di.label
                    font.pixelSize: 14; font.weight: Font.Medium
                    color: di.isActive ? textPrimary : "#ccd0d8"
                }
                Text {
                    text: di.subtitle
                    font.pixelSize: 10
                    color: di.isActive ? accentCyan : textDim
                    visible: di.subtitle.length > 0 && !di.compact
                }
            }

            Item { width: 1; height: 1; Layout.fillWidth: true }

            // Active indicator
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 6; height: 6; radius: 3
                color: accentCyan
                visible: di.isActive
            }
        }

        MouseArea {
            id: diArea
            anchors.fill: parent
            onClicked: di.tapped()
        }

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    // ═══════════════════════════════════════════════════════════
    // FORMULA SELECTION POPUP
    // ═══════════════════════════════════════════════════════════
    Popup {
        id: formulaPopup
        anchors.centerIn: parent
        width: Math.min(root.width * 0.92, 420)
        height: Math.min(root.height * 0.78, 580)
        modal: true; dim: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale"; from: 0.94; to: 1; duration: 200; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150; easing.type: Easing.InCubic }
        }

        background: Rectangle {
            color: bgDark
            radius: 16
            border.color: borderSubtle; border.width: 1

            // Top accent bar
            Rectangle {
                width: parent.width; height: 2; radius: 16
                anchors.top: parent.top
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: accentCyan }
                    GradientStop { position: 0.5; color: accentPurple }
                    GradientStop { position: 1.0; color: accentMagenta }
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            anchors.topMargin: 22
            spacing: 12

            // Title
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Formula Library"
                font.pixelSize: 20; font.bold: true
                font.letterSpacing: 1
                color: textPrimary
            }

            // Count badge
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: countText.implicitWidth + 24
                height: 26; radius: 13
                color: Qt.rgba(0, 0.82, 1, 0.08)
                border.color: Qt.rgba(0, 0.82, 1, 0.15); border.width: 1

                Text {
                    id: countText
                    anchors.centerIn: parent
                    text: bridge ? (bridge.formulaCount + " fractals") : ""
                    font.pixelSize: 11; font.weight: Font.Medium
                    color: accentCyan
                }
            }

            // Formula list
            ListView {
                id: formulaList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: bridge ? bridge.formulaCount : 0
                spacing: 3

                delegate: Rectangle {
                    width: formulaList.width
                    height: 50; radius: 10
                    color: fArea.pressed ? Qt.rgba(0, 0.82, 1, 0.08) : bgCard
                    border.color: fArea.pressed ? Qt.rgba(0, 0.82, 1, 0.2) : borderSubtle
                    border.width: 1

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        spacing: 12

                        // Index badge
                        Rectangle {
                            width: 30; height: 30
                            anchors.verticalCenter: parent.verticalCenter
                            radius: 8
                            color: Qt.rgba(1,1,1,0.04)
                            Text {
                                anchors.centerIn: parent
                                text: (index + 1).toString()
                                font.pixelSize: 11; font.weight: Font.Medium
                                font.family: "monospace"
                                color: textDim
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: bridge ? bridge.getFormulaName(index) : ""
                            font.pixelSize: 14; font.weight: Font.Medium
                            color: textPrimary
                        }
                    }

                    MouseArea {
                        id: fArea
                        anchors.fill: parent
                        onClicked: {
                            if (bridge) bridge.setFormula(index)
                            formulaPopup.close()
                        }
                    }

                    Behavior on color { ColorAnimation { duration: 100 } }
                    Behavior on border.color { ColorAnimation { duration: 100 } }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    contentItem: Rectangle {
                        implicitWidth: 3; radius: 1.5
                        color: Qt.rgba(0, 0.82, 1, 0.3)
                    }
                }
            }
        }
    }
}
