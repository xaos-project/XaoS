import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import "."

Item {
    id: root
    anchors.fill: parent

    // Short local names for the Theme tokens, kept so the bindings throughout
    // this file stay readable. Theme.qml is the single source of truth.
    readonly property color bgDark:        Theme.bgDark
    readonly property color bgCard:        Theme.bgCard
    readonly property color bgSurface:     Theme.bgSurface
    readonly property color accentCyan:    Theme.accentCyan
    readonly property color accentMagenta: Theme.accentMagenta
    readonly property color accentPurple:  Theme.accentPurple
    readonly property color accentGreen:   Theme.accentGreen
    readonly property color accentAmber:   Theme.accentAmber
    readonly property color textPrimary:   Theme.textPrimary
    readonly property color textSecondary: Theme.textSecondary
    readonly property color textDim:       Theme.textDim
    readonly property color borderSubtle:  Theme.borderSubtle
    readonly property color borderBright:  Theme.borderBright

    readonly property bool isWide: width >= Theme.wideBreakpoint
    readonly property real panelWidth: Math.min(width * 0.94, Theme.maxPanelWidth)
    // Edge padding and chrome sizes step up once there is room for them.
    readonly property int  edgeMargin: isWide ? Theme.s5 : Theme.s3

    property int  currentTab:0      
    property bool juliaActive:bridge ? bridge.juliaMode > 0 : false
    property bool formulasPopupVisible: false

    function hsvToColor(h, s, v) {
        var i = Math.floor(h * 6)
        var f = h * 6 - i
        var p = v * (1 - s)
        var q = v * (1 - f * s)
        var tt = v * (1 - (1 - f) * s)
        var r, g, b
        switch (i % 6) {
            case 0: r = v; g = tt; b = p; break
            case 1: r = q; g = v; b = p; break
            case 2: r = p; g = v; b = tt; break
            case 3: r = p; g = q; b = v; break
            case 4: r = tt; g = p; b = v; break
            default: r = v; g = p; b = q; break
        }
        return Qt.rgba(r, g, b, 1)
    }

    function generatePaletteColor(alg, seed, shift, index, count) {
        var t = count > 1 ? index / (count - 1) : 0
        var s = (Math.round(seed) * 9301 + 49297) % 233280
        var baseHue = s / 233280.0

        var hue
        if (alg === 2) {
            hue = baseHue + Math.sin(t * Math.PI * 4) * 0.5
        } else if (alg === 3) {
            var tri = Math.abs(((t * 4) % 2) - 1)
            hue = baseHue + tri * 0.7
        } else {
            hue = baseHue + t
        }
        hue += shift / 255.0
        hue = hue - Math.floor(hue)

        var sat = 0.55 + 0.35 * Math.abs(Math.sin((baseHue + t) * Math.PI * 3))
        var val = 0.55 + 0.40 * Math.abs(Math.sin((t + baseHue) * Math.PI * 2))

        return hsvToColor(hue, Math.min(sat, 1), Math.min(Math.max(val, 0.35), 1))
    }



    MultiPointTouchArea {
        id: touchArea
        anchors.fill: parent
        z: 0
        mouseEnabled: true
        minimumTouchPoints: 1
        maximumTouchPoints: 2
        visible: currentTab === 0 || currentTab === 1
        touchPoints: [
            TouchPoint { id: tp1 },
            TouchPoint { id: tp2 }
        ]

        property bool  isPinching:       false
        property real  initialDistance:  0.0
        property real  dragStartX:       0
        property real  dragStartY:       0
        property bool  isDragging:       false
        property real  lastTapTime:      0

        onPressed: function(touchPoints) {
            if (touchPoints.length === 2) {
                isPinching = true
                initialDistance = ptDist(tp1, tp2)
                bridge.gesturePinchStarted()
            } else if (touchPoints.length === 1) {
                isPinching = false
                dragStartX = touchPoints[0].x
                dragStartY = touchPoints[0].y
                isDragging = false
                if (bridge) bridge.updatePointerPosition(touchPoints[0].x, touchPoints[0].y)
            }
        }

        onUpdated: function(touchPoints) {
            if (isPinching && tp1.pressed && tp2.pressed) {
                var d = ptDist(tp1, tp2)
                var scale = d / initialDistance
                bridge.gesturePinch(scale, (tp1.x + tp2.x) / 2, (tp1.y + tp2.y) / 2)
            } else if (touchPoints.length === 1 && !isPinching) {
                var dx = touchPoints[0].x - dragStartX
                var dy = touchPoints[0].y - dragStartY
                if (bridge) bridge.updatePointerPosition(touchPoints[0].x, touchPoints[0].y)
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
                var now = Date.now()
                if (now - lastTapTime < 350) {
                    lastTapTime = 0
                    if (root.juliaActive && bridge) {
                        bridge.toggleMandelbrot()
                    }
                } else {
                    lastTapTime = now
                    bridge.startZoomIn()
                    zoomPulseTimer.restart()
                }
            }
        }

        function ptDist(p1, p2) {
            var dx = p2.x - p1.x; var dy = p2.y - p1.y
            return Math.sqrt(dx * dx + dy * dy)
        }
    }

    Timer {
        id: zoomPulseTimer
        interval: 200
        onTriggered: bridge.stopZoom()
    }

    Item {
        id: screenStack
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomNav.top
        z: 5

        Item {
            id: exploreScreen
            anchors.fill: parent
            visible: currentTab === 0

            Item {
                id: topBar
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: root.isWide ? 60 : 52
                z: 20

                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Theme.alpha(Theme.bgDark, 0.88) }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "XAOS"
                    font.pixelSize: root.isWide ? 17 : 15
                    font.bold: true
                    font.letterSpacing: 5
                    color: textPrimary
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: root.edgeMargin
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    IconButton {
                        icon: "share"
                        accent: accentGreen
                        onClicked: shareDialog.open()
                    }
                }
            }

            Column {
                id: statsOverlay
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.rightMargin: root.edgeMargin
                anchors.bottomMargin: 10
                spacing: 4
                z: 15

                StatPill {
                    label: "FRAC"; value: bridge ? bridge.formulaName : "—"
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: formulasPopupVisible = true
                    }
                }
                StatPill { label: "ITER"; value: bridge ? bridge.maxIterations.toString() : "—" }
                StatPill { label: "ZOOM"; value: bridge ? bridge.zoomLevel : "1.00×" }
            }

            Column {
                id: zoomControls
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: root.edgeMargin
                spacing: root.isWide ? 10 : 8
                z: 15

                ZoomFab {
                    icon: "play_circle"
                    holdable: false
                    active: bridge ? bridge.autopilotActive : false
                    onAction: { if (bridge) bridge.toggleAutopilot() }
                }

                Rectangle {
                    width: 28; height: 1
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: borderBright
                }

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

                Rectangle {
                    width: 28; height: 1
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: borderBright
                }

                ZoomFab {
                    icon: "home"
                    holdable: false
                    onAction: { if (bridge) bridge.resetView() }
                }
            }

            Row {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                anchors.left: parent.left
                anchors.leftMargin: root.edgeMargin
                spacing: 8
                z: 15

                StatusBadge {
                    visible: bridge ? bridge.autopilotActive : false
                    dotColor: accentCyan
                    labelText: "AUTOPILOT"
                    badgeColor: Theme.alpha(Theme.accentCyan, 0.10)
                    badgeBorder: Theme.alpha(Theme.accentCyan, 0.28)
                    pulseDot: true
                }

                StatusBadge {
                    visible: root.juliaActive
                    dotColor: accentMagenta
                    labelText: root.juliaActive && bridge && !bridge.isMandelbrot ? "JULIA (FULL)" : "JULIA"
                    badgeColor: Theme.alpha(Theme.accentMagenta, 0.10)
                    badgeBorder: Theme.alpha(Theme.accentMagenta, 0.28)
                    pulseDot: false
                }
            }
        }


        Rectangle {
            id: paletteScreen
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.top: root.isWide ? parent.top : undefined
            width: root.isWide ? root.panelWidth : parent.width
            height: root.isWide ? parent.height
                                : Math.min(parent.height * 0.62, 540)
            visible: currentTab === 1
            color: bgDark
            border.color: borderBright; border.width: 1
            // On phones this is a bottom sheet, so only the top corners round.
            // Per-corner radius needs Qt 6.7, which the Android build may not
            // have, so square off the bottom with a masking rectangle instead.
            radius: root.isWide ? 0 : Theme.radiusXl

            Rectangle {
                visible: !root.isWide
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.radius
                color: parent.color
                border.color: parent.border.color
                border.width: parent.border.width

                // Hide the masking rectangle's own top edge.
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 1
                    height: 2
                    color: paletteScreen.color
                }
            }

            ListModel {
                id: palettePresets
                ListElement { alg: 1; seed: 12345 }
                ListElement { alg: 2; seed: 8421 }
                ListElement { alg: 1; seed: 3500 }
                ListElement { alg: 3; seed: 19000 }
                ListElement { alg: 2; seed: 25000 }
                ListElement { alg: 3; seed: 7777 }
                ListElement { alg: 1; seed: 42 }
                ListElement { alg: 2; seed: 15000 }
            }

            property int selectedPreset: -1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.isWide ? 0 : 16
                    visible: !root.isWide

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 6
                        width: 36; height: 4; radius: 2
                        color: borderBright
                    }
                }

                ScreenHeader {
                    subtitle: "COLOR PALETTE"
                    title: "Palette"
                    showCount: false
                    wide: root.isWide
                }

                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentHeight: palCol.implicitHeight + 20
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: Basic.ScrollBar {
                        policy: ScrollBar.AsNeeded
                        contentItem: Rectangle {
                            implicitWidth: 3; radius: 1.5
                            color: Theme.alpha(Theme.accentCyan, 0.25)
                        }
                    }

                    ColumnLayout {
                        id: palCol
                        width: parent.width
                        spacing: 0

                        SectionLabel { text: "PRESETS" }

                        GridView {
                            id: palGrid
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            Layout.preferredHeight: Math.ceil(palettePresets.count / 2) * 74
                            clip: true
                            interactive: false
                            cellWidth: (width - 10) / 2
                            cellHeight: 74
                            model: palettePresets

                            delegate: Item {
                                id: presetDelegate
                                property int presetAlg: model.alg
                                property int presetSeed: model.seed
                                width: palGrid.cellWidth
                                height: palGrid.cellHeight

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 5
                                    radius: 13
                                    border.color: paletteScreen.selectedPreset === index
                                                  ? accentCyan
                                                  : presetArea.containsMouse
                                                    ? Theme.alpha(Theme.accentCyan, 0.45)
                                                    : borderSubtle
                                    border.width: paletteScreen.selectedPreset === index ? 2 : 1
                                    clip: true

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 13
                                        clip: true
                                        
                                        Row {
                                            anchors.fill: parent
                                            spacing: 0
                                            Repeater {
                                                model: bridge ? bridge.getPalettePreview(presetDelegate.presetAlg, presetDelegate.presetSeed, 0) : []
                                                Rectangle {
                                                    width: parent.width / 100
                                                    height: parent.height
                                                    color: modelData
                                                }
                                            }
                                        }
                                    }

                                    MouseArea {
                                        id: presetArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            paletteScreen.selectedPreset = index
                                            algInput.value = model.alg
                                            algSlider.value = model.alg
                                            seedInput.value = model.seed
                                            seedSlider.value = model.seed
                                            shiftInput.value = 0
                                            shiftSlider.value = 0
                                            if (bridge) bridge.setCustomPalette(model.alg, model.seed, 0)
                                        }
                                    }

                                    Behavior on border.color { ColorAnimation { duration: 180 } }
                                }
                            }
                        }

                        Connections {
                            target: bridge
                            function onStateChanged() {
                                if (algInput && seedInput && shiftInput && algSlider && seedSlider && shiftSlider) {
                                    if (algInput.value !== bridge.paletteAlgorithm) algInput.value = bridge.paletteAlgorithm;
                                    if (algSlider.value !== bridge.paletteAlgorithm) algSlider.value = bridge.paletteAlgorithm;
                                    if (seedInput.value !== bridge.paletteSeed) seedInput.value = bridge.paletteSeed;
                                    if (seedSlider.value !== bridge.paletteSeed) seedSlider.value = bridge.paletteSeed;
                                    if (shiftInput.value !== bridge.paletteShift) shiftInput.value = bridge.paletteShift;
                                    if (shiftSlider.value !== bridge.paletteShift) shiftSlider.value = bridge.paletteShift;
                                }
                            }
                        }

                        SectionLabel { text: "CUSTOM PALETTE" }

                        SettingsCard {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16

                            Column {
                                width: parent.width
                                spacing: 10
                                RowLayout {
                                    width: parent.width
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Algorithm"
                                        font.pixelSize: 13; font.weight: Font.Medium
                                        color: textPrimary
                                    }
                                    ThemedSpinBox {
                                        id: algInput
                                        Layout.preferredWidth: 140
                                        from: 1; to: 3; stepSize: 1
                                        value: bridge ? bridge.paletteAlgorithm : 1
                                        editable: true
                                        onValueModified: {
                                            algSlider.value = value
                                            paletteScreen.selectedPreset = -1
                                            if (bridge) bridge.setCustomPalette(value, seedInput.value, shiftInput.value)
                                        }
                                    }
                                }
                                StyledSlider {
                                    id: algSlider
                                    width: parent.width
                                    from: 1; to: 3; stepSize: 1
                                    value: bridge ? bridge.paletteAlgorithm : 1
                                    onMoved: {
                                        algInput.value = value
                                        paletteScreen.selectedPreset = -1
                                        if (bridge) bridge.setCustomPalette(Math.round(value), seedInput.value, shiftInput.value)
                                    }
                                }
                            }
                        }

                        SettingsCard {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            Layout.topMargin: 8

                            Column {
                                width: parent.width
                                spacing: 10
                                RowLayout {
                                    width: parent.width
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Seed"
                                        font.pixelSize: 13; font.weight: Font.Medium
                                        color: textPrimary
                                    }
                                    ThemedSpinBox {
                                        id: seedInput
                                        Layout.preferredWidth: 140
                                        from: 0; to: 65535; stepSize: 1
                                        value: bridge ? bridge.paletteSeed : 0
                                        editable: true
                                        onValueModified: {
                                            seedSlider.value = value
                                            paletteScreen.selectedPreset = -1
                                            if (bridge) bridge.setCustomPalette(algInput.value, value, shiftInput.value)
                                        }
                                    }
                                }
                                StyledSlider {
                                    id: seedSlider
                                    width: parent.width
                                    from: 0; to: 65535; stepSize: 1
                                    value: bridge ? bridge.paletteSeed : 0
                                    onMoved: {
                                        seedInput.value = value
                                        paletteScreen.selectedPreset = -1
                                        if (bridge) bridge.setCustomPalette(algInput.value, Math.round(value), shiftInput.value)
                                    }
                                }
                            }
                        }

                        SettingsCard {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            Layout.topMargin: 8

                            Column {
                                width: parent.width
                                spacing: 10
                                RowLayout {
                                    width: parent.width
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Shift"
                                        font.pixelSize: 13; font.weight: Font.Medium
                                        color: textPrimary
                                    }
                                    ThemedSpinBox {
                                        id: shiftInput
                                        Layout.preferredWidth: 140
                                        from: 0; to: 65534; stepSize: 1
                                        value: bridge ? bridge.paletteShift : 0
                                        editable: true
                                        onValueModified: {
                                            shiftSlider.value = value
                                            paletteScreen.selectedPreset = -1
                                            if (bridge) bridge.setCustomPalette(algInput.value, seedInput.value, value)
                                        }
                                    }
                                }
                                StyledSlider {
                                    id: shiftSlider
                                    width: parent.width
                                    from: 0; to: 65534; stepSize: 1
                                    value: bridge ? bridge.paletteShift : 0
                                    onMoved: {
                                        shiftInput.value = value
                                        paletteScreen.selectedPreset = -1
                                        if (bridge) bridge.setCustomPalette(algInput.value, seedInput.value, Math.round(value))
                                    }
                                }
                            }
                        }

                        Item { Layout.preferredHeight: 14 }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            height: 48; radius: 13
                            clip: true
                            border.color: borderSubtle; border.width: 1
                            Row {
                                anchors.fill: parent
                                spacing: 0
                                Repeater {
                                    model: bridge ? bridge.getPalettePreview(algInput.value, seedInput.value, shiftInput.value) : []
                                    Rectangle {
                                        width: parent.width / 100
                                        height: parent.height
                                        color: modelData
                                    }
                                }
                            }

                            Text {
                                anchors.left: parent.left; anchors.leftMargin: 10
                                anchors.top: parent.top; anchors.topMargin: 6
                                text: "PREVIEW"
                                font.pixelSize: 8; font.bold: true; font.letterSpacing: 2
                                color: Qt.rgba(1, 1, 1, 0.85)
                                style: Text.Outline
                                styleColor: Qt.rgba(0, 0, 0, 0.55)
                            }
                        }

                        Item { Layout.preferredHeight: 14 }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            Layout.bottomMargin: 12
                            height: 48; radius: 13
                            color: randomArea.pressed ? Theme.alpha(Theme.accentCyan, 0.20)
                                                      : randomArea.containsMouse
                                                        ? Theme.alpha(Theme.accentCyan, 0.13)
                                                        : Theme.alpha(Theme.accentCyan, 0.07)
                            border.color: randomArea.containsMouse
                                          ? Theme.alpha(Theme.accentCyan, 0.45)
                                          : Theme.alpha(Theme.accentCyan, 0.22)
                            border.width: 1

                            Row {
                                anchors.centerIn: parent
                                spacing: 8
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "casino"
                                    font.family: Theme.iconFont
                                    font.pixelSize: 20
                                    color: accentCyan
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "Randomise Palette"
                                    font.pixelSize: 13; font.weight: Font.Medium
                                    color: accentCyan
                                }
                            }

                            MouseArea {
                                id: randomArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    paletteScreen.selectedPreset = -1
                                    if (bridge) bridge.randomizePalette()
                                }
                            }

                            Behavior on color { ColorAnimation { duration: 120 } }
                        }

                        Item { Layout.preferredHeight: 8 }
                    }
                }
            }
        }

        Rectangle {
            id: settingsScreen
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.isWide ? root.panelWidth : parent.width
            visible: currentTab === 2
            color: bgDark
            border.color: borderBright; border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                ScreenHeader {
                    subtitle: "PREFERENCES"
                    title: "Settings"
                    showCount: false
                    wide: root.isWide
                }

                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentHeight: settingsCol.implicitHeight + 30
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: Basic.ScrollBar {
                        policy: ScrollBar.AsNeeded
                        contentItem: Rectangle {
                            implicitWidth: 3; radius: 1.5
                            color: Theme.alpha(Theme.accentCyan, 0.25)
                        }
                    }

                    ColumnLayout {
                        id: settingsCol
                        width: parent.width
                        spacing: 0

                        Item { Layout.preferredHeight: 14 }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 20; Layout.rightMargin: 20
                            
                            Column {
                                Layout.fillWidth: true
                                spacing: 3
                                Text {
                                    text: "CURRENT FRACTAL"
                                    font.pixelSize: 9; font.letterSpacing: 2
                                    font.weight: Font.Medium; color: textDim
                                }
                                Text {
                                    text: bridge ? bridge.formulaName : "Mandelbrot"
                                    font.pixelSize: 15; font.bold: true; color: accentCyan
                                }
                            }
                            
                            Column {
                                Layout.alignment: Qt.AlignRight
                                spacing: 3
                                Text {
                                    text: "ABOUT XAOS"
                                    font.pixelSize: 9; font.letterSpacing: 2
                                    font.weight: Font.Medium; color: textDim
                                    anchors.right: parent.right
                                }
                                Text {
                                    text: bridge ? ("v" + bridge.version) : "v?"
                                    font.pixelSize: 15; font.bold: true; color: textPrimary
                                    anchors.right: parent.right
                                }
                            }
                        }
                        
                        Item { Layout.preferredHeight: 6 }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            height: 1
                            color: borderSubtle
                        }
                        Item { Layout.preferredHeight: 2 }

                        SectionLabel { text: "RENDERING & COMPUTE" }

                        SettingsCard {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16

                            Column {
                                width: parent.width
                                spacing: 10

                                Row {
                                    width: parent.width
                                    Text {
                                        text: "Iteration Depth"
                                        font.pixelSize: 13; font.weight: Font.Medium
                                        color: textPrimary
                                        width: parent.width - iterValText.implicitWidth
                                    }
                                    Text {
                                        id: iterValText
                                        text: bridge ? bridge.maxIterations.toString() : "256"
                                        font.pixelSize: 12; font.bold: true
                                        font.family: "monospace"
                                        color: accentMagenta
                                    }
                                }

                                StyledSlider {
                                    id: iterSlider
                                    width: parent.width
                                    from: 10; to: 5000; stepSize: 10
                                    value: bridge ? bridge.maxIterations : 256
                                    onMoved: { if (bridge) bridge.setIterations(Math.round(value)) }
                                }
                            }
                        }



                        SectionLabel { text: "NAVIGATION" }

                        SettingsCard {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16

                            Column {
                                width: parent.width

                                SettingsActionRow {
                                    width: parent.width
                                    icon: "casino"
                                    label: "Random Example"
                                    subtitle: "Load a preset location"
                                    iconColor: accentAmber
                                    iconBg: Theme.alpha(Theme.accentAmber, 0.10)
                                    iconBorder: Theme.alpha(Theme.accentAmber, 0.18)
                                    showDivider: true
                                    onTapped: { if (bridge) bridge.loadRandomExample() }
                                }

                                SettingsActionRow {
                                    width: parent.width
                                    icon: "home"
                                    label: "Reset View"
                                    subtitle: "Return to default position"
                                    iconColor: accentCyan
                                    iconBg: Theme.alpha(Theme.accentCyan, 0.10)
                                    iconBorder: Theme.alpha(Theme.accentCyan, 0.18)
                                    showDivider: false
                                    onTapped: { if (bridge) bridge.resetView() }
                                }
                            }
                        }



                        Item { Layout.preferredHeight: 16 }
                    }
                }
            }
        }

        Item {
            id: communityScreen
            anchors.fill: parent
            visible: currentTab === 3

            CommunityGallery {
                id: communityGallery
            }
        }
    }

    Item {
        id: formulasPopup
        anchors.fill: parent
        z: 40
        opacity: formulasPopupVisible ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        property var filteredFormulaIndices: []

        function updateFormulaFilter() {
            var result = []
            var query = formulaSearch.text.toLowerCase()
            var count = bridge ? bridge.formulaCount : 0
            for (var i = 0; i < count; i++) {
                var name = bridge.getFormulaName(i)
                if (query.length === 0 || name.toLowerCase().indexOf(query) !== -1) {
                    result.push(i)
                }
            }
            filteredFormulaIndices = result
        }

        onVisibleChanged: {
            if (visible) {
                formulaSearch.text = ""
                updateFormulaFilter()
            }
        }

        Rectangle {
            anchors.fill: parent
            color: Theme.scrim

            MouseArea {
                anchors.fill: parent
                onClicked: formulasPopupVisible = false
            }
        }

        readonly property real kbOffset: {
            if (!Qt.inputMethod.visible) return 0
            var h = Qt.inputMethod.keyboardRectangle.height
            if (h > root.height) h = h / Screen.devicePixelRatio
            return Math.min(h, root.height * 0.55)
        }

        Rectangle {
            id: formulasSheet
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: root.isWide ? undefined : parent.bottom
            anchors.bottomMargin: root.isWide ? 0 : formulasPopup.kbOffset
            anchors.verticalCenter: root.isWide ? parent.verticalCenter : undefined
            width: root.isWide ? Math.min(parent.width * 0.86, 480) : parent.width
            height: root.isWide ? Math.min(parent.height * 0.80, 600)
                                : Math.min(Math.min(parent.height * 0.72, 540),
                                           parent.height - formulasPopup.kbOffset - 8)

            Behavior on anchors.bottomMargin {
                NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
            }
            radius: 20
            color: bgDark
            border.color: borderBright; border.width: 1

            transform: Translate {
                y: formulasPopupVisible ? 0 : 36
                Behavior on y { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            }
            MouseArea { anchors.fill: parent; onClicked: {} }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.isWide ? 10 : 20

                    Rectangle {
                        visible: !root.isWide
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top; anchors.topMargin: 8
                        width: 36; height: 4; radius: 2
                        color: borderBright
                    }
                }

                ScreenHeader {
                    subtitle: "FORMULA LIBRARY"
                    title: "Choose a Fractal"
                    countText: bridge
                               ? (formulaSearch.text.length > 0
                                  ? (formulasPopup.filteredFormulaIndices.length + " results")
                                  : (bridge.formulaCount + " fractals"))
                               : "—"
                    showCount: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    Layout.topMargin: 10; Layout.bottomMargin: 4
                    height: 40; radius: 11
                    color: bgCard
                    border.color: borderBright; border.width: 1

                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: "search"
                        font.family: Theme.iconFont
                        font.pixelSize: 20
                        color: textDim
                    }

                    TextInput {
                        id: formulaSearch
                        anchors.left: parent.left; anchors.leftMargin: 40
                        anchors.right: parent.right; anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        color: textPrimary
                        font.pixelSize: 13
                        clip: true
                        onTextChanged: formulasPopup.updateFormulaFilter()

                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 0
                            verticalAlignment: Text.AlignVCenter
                            text: "Search fractals…"
                            font.pixelSize: 13
                            color: textDim
                            visible: formulaSearch.text.length === 0
                        }
                    }
                }

                Rectangle {
                    id: userFormulaCard
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    Layout.topMargin: 6
                    Layout.preferredHeight: userFormulaExpanded
                                            ? userFormulaCol.implicitHeight + 16
                                            : 40
                    Behavior on Layout.preferredHeight {
                        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                    }
                    clip: true
                    radius: 12
                    color: bgCard
                    border.color: userFormulaExpanded ? accentCyan : borderSubtle
                    border.width: 1

                    property bool userFormulaExpanded: false

                    function applyUserFormula() {
                        if (!bridge) return
                        var userIdx = bridge.formulaCount - 1
                        bridge.setFormula(userIdx)
                        bridge.setUserFormula(userFormulaInput.text)
                        if (userInitialInput.text.length > 0)
                            bridge.setUserInitial(userInitialInput.text)
                        Qt.inputMethod.hide()
                    }

                    Column {
                        id: userFormulaCol
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 0
                        spacing: 0

                        Rectangle {
                            width: parent.width; height: 40
                            color: "transparent"

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 12
                                spacing: 8

                                Text {
                                    text: "functions"
                                    font.family: Theme.iconFont; font.pixelSize: 18
                                    color: accentCyan
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    text: "User Formula"
                                    font.pixelSize: 13; font.weight: Font.Medium
                                    color: textPrimary
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            Text {
                                anchors.right: parent.right; anchors.rightMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                                text: userFormulaCard.userFormulaExpanded
                                      ? "expand_less" : "expand_more"
                                font.family: Theme.iconFont; font.pixelSize: 20
                                color: textSecondary
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: userFormulaCard.userFormulaExpanded =
                                           !userFormulaCard.userFormulaExpanded
                            }
                        }

                        Rectangle {
                            width: parent.width - 24; height: 32
                            anchors.horizontalCenter: parent.horizontalCenter
                            radius: 8; color: Theme.alpha(Theme.accentCyan, 0.06)
                            border.color: Theme.alpha(Theme.accentCyan, 0.15); border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: bridge ? (bridge.userFormulaText || "z^2+c") : "z^2+c"
                                font.pixelSize: 13; font.family: "monospace"
                                font.weight: Font.DemiBold
                                color: accentCyan
                                elide: Text.ElideMiddle
                                width: parent.width - 16
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        Item { width: 1; height: 8 }

                        Text {
                            anchors.left: parent.left; anchors.leftMargin: 14
                            text: "FORMULA  f(z, c, p, n)"
                            font.pixelSize: 9; font.bold: true
                            font.letterSpacing: 2
                            color: textDim
                        }
                        Item { width: 1; height: 4 }

                        Rectangle {
                            width: parent.width - 24; height: 36
                            anchors.horizontalCenter: parent.horizontalCenter
                            radius: 9; color: bgSurface
                            border.color: userFormulaInput.activeFocus
                                          ? accentCyan : borderSubtle
                            border.width: 1

                            TextInput {
                                id: userFormulaInput
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                color: textPrimary
                                font.pixelSize: 13; font.family: "monospace"
                                clip: true
                                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                                Keys.onReturnPressed: userFormulaCard.applyUserFormula()
                                Keys.onEnterPressed: userFormulaCard.applyUserFormula()

                                Component.onCompleted: {
                                    text = bridge ? (bridge.userFormulaText || "") : ""
                                }
                                Connections {
                                    target: formulasPopup
                                    function onVisibleChanged() {
                                        if (formulasPopup.visible)
                                            userFormulaInput.text = bridge
                                                ? (bridge.userFormulaText || "") : ""
                                    }
                                }

                                Text {
                                    anchors.fill: parent
                                    verticalAlignment: Text.AlignVCenter
                                    text: "e.g. z^2+c"
                                    font.pixelSize: 13; font.family: "monospace"
                                    color: textDim
                                    visible: userFormulaInput.text.length === 0
                                }
                            }
                        }

                        Item { width: 1; height: 8 }

                        Text {
                            anchors.left: parent.left; anchors.leftMargin: 14
                            text: "INITIAL VALUE  z₀"
                            font.pixelSize: 9; font.bold: true
                            font.letterSpacing: 2
                            color: textDim
                        }
                        Item { width: 1; height: 4 }

                        Rectangle {
                            width: parent.width - 24; height: 36
                            anchors.horizontalCenter: parent.horizontalCenter
                            radius: 9; color: bgSurface
                            border.color: userInitialInput.activeFocus
                                          ? accentCyan : borderSubtle
                            border.width: 1

                            TextInput {
                                id: userInitialInput
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                color: textPrimary
                                font.pixelSize: 13; font.family: "monospace"
                                clip: true
                                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                                Keys.onReturnPressed: userFormulaCard.applyUserFormula()
                                Keys.onEnterPressed: userFormulaCard.applyUserFormula()

                                Component.onCompleted: {
                                    text = bridge ? (bridge.userInitialText || "0") : "0"
                                }
                                Connections {
                                    target: formulasPopup
                                    function onVisibleChanged() {
                                        if (formulasPopup.visible)
                                            userInitialInput.text = bridge
                                                ? (bridge.userInitialText || "0") : "0"
                                    }
                                }

                                Text {
                                    anchors.fill: parent
                                    verticalAlignment: Text.AlignVCenter
                                    text: "0"
                                    font.pixelSize: 13; font.family: "monospace"
                                    color: textDim
                                    visible: userInitialInput.text.length === 0
                                }
                            }
                        }

                        Item { width: 1; height: 10 }

                        Rectangle {
                            width: parent.width - 24; height: 38
                            anchors.horizontalCenter: parent.horizontalCenter
                            radius: 10
                            color: applyArea.pressed
                                   ? Theme.alpha(Theme.accentCyan, 0.25)
                                   : Theme.alpha(Theme.accentCyan, 0.12)
                            border.color: accentCyan; border.width: 1

                            Row {
                                anchors.centerIn: parent; spacing: 6
                                Text {
                                    text: "play_arrow"
                                    font.family: Theme.iconFont; font.pixelSize: 18
                                    color: accentCyan
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    text: "Apply Formula"
                                    font.pixelSize: 13; font.weight: Font.DemiBold
                                    color: accentCyan
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            MouseArea {
                                id: applyArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: userFormulaCard.applyUserFormula()
                            }

                            Behavior on color { ColorAnimation { duration: 120 } }
                        }

                        Item { width: 1; height: 8 }
                    }
                }

                ListView {
                    id: formulaList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    Layout.topMargin: 4
                    clip: true
                    spacing: 5
                    model: formulasPopup.filteredFormulaIndices

                    ScrollBar.vertical: Basic.ScrollBar {
                        policy: ScrollBar.AsNeeded
                        contentItem: Rectangle {
                            implicitWidth: 3; radius: 1.5
                            color: Theme.alpha(Theme.accentCyan, 0.25)
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: formulaList.count === 0
                        text: "No fractals match your search"
                        font.pixelSize: 13
                        color: textDim
                    }

                    delegate: Rectangle {
                        id: fItem
                        property int realIndex: modelData
                        property bool isSelected: bridge ? (bridge.getFormulaName(realIndex) === bridge.formulaName) : false
                        width: formulaList.width
                        height: 52; radius: 11
                        color: fArea.pressed
                               ? Theme.alpha(Theme.accentCyan, 0.10)
                               : isSelected
                                 ? Theme.alpha(Theme.accentCyan, 0.07)
                                 : fArea.containsMouse
                                   ? Theme.alpha(Theme.accentCyan, 0.04)
                                   : bgCard
                        border.color: isSelected
                                      ? Theme.alpha(Theme.accentCyan, 0.35)
                                      : (fArea.pressed || fArea.containsMouse)
                                        ? Theme.alpha(Theme.accentCyan, 0.2)
                                        : borderSubtle
                        border.width: 1

                        Rectangle {
                            id: idxBadge
                            x: 12; anchors.verticalCenter: parent.verticalCenter
                            width: 30; height: 30; radius: 9
                            color: Qt.rgba(1, 1, 1, 0.04)
                            Text {
                                anchors.centerIn: parent
                                text: (fItem.realIndex + 1).toString()
                                font.pixelSize: 10; font.family: "monospace"
                                font.weight: Font.Medium
                                color: textDim
                            }
                        }

                        Text {
                            anchors.left: idxBadge.right; anchors.leftMargin: 10
                            anchors.right: checkMark.left; anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: bridge ? bridge.getFormulaName(fItem.realIndex) : ""
                            font.pixelSize: 14; font.weight: Font.Medium
                            color: fItem.isSelected || fArea.containsMouse
                                   ? textPrimary : Theme.alpha(Theme.textPrimary, 0.82)
                            elide: Text.ElideRight
                        }

                        Text {
                            id: checkMark
                            anchors.right: parent.right; anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter
                            text: "check"
                            font.family: Theme.iconFont
                            font.pixelSize: 20
                            color: accentCyan
                            visible: fItem.isSelected
                        }

                        MouseArea {
                            id: fArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (bridge) bridge.setFormula(fItem.realIndex)
                                formulasPopupVisible = false
                            }
                        }

                        Behavior on color       { ColorAnimation { duration: 100 } }
                        Behavior on border.color { ColorAnimation { duration: 100 } }
                    }
                }

                Item { Layout.preferredHeight: 8 }
            }
        }
    }

    Rectangle {
        id: bottomNav
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.isWide ? 64 : 60
        z: 30
        color: Theme.alpha(Theme.bgDark, 0.96)
        border.color: borderSubtle; border.width: 1

        // On a wide window the six tabs would each be hundreds of pixels
        // across, so the row is capped and centred instead.
        readonly property real tabRowWidth: Math.min(root.width, Theme.maxNavWidth)

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left; anchors.right: parent.right
            height: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.2; color: Theme.alpha(Theme.accentCyan, 0.30) }
                GradientStop { position: 0.5; color: Theme.alpha(Theme.accentCyan, 0.55) }
                GradientStop { position: 0.8; color: Theme.alpha(Theme.accentCyan, 0.30) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: bottomNav.tabRowWidth

            NavTab {
                tabIndex: 0; icon: "explore";  label: "Explore"
                active: currentTab === 0 && !formulasPopupVisible
                onTapped: { formulasPopupVisible = false; currentTab = 0 }
            }
            NavTab {
                tabIndex: 1; icon: "functions"; label: "Formulas"
                active: formulasPopupVisible
                onTapped: {
                    currentTab = 0
                    formulasPopupVisible = !formulasPopupVisible
                }
            }
            NavTab {
                tabIndex: 2; icon: "palette";   label: "Palette"
                active: currentTab === 1 && !formulasPopupVisible
                onTapped: { formulasPopupVisible = false; currentTab = 1 }
            }
            NavTab {
                tabIndex: 3; icon: "group";   label: "Community"
                active: currentTab === 3 && !formulasPopupVisible
                onTapped: { formulasPopupVisible = false; currentTab = 3 }
            }
            NavTab {
                tabIndex: 4; icon: "blur_on";   label: "Julia"
                active: root.juliaActive
                onTapped: {
                    if (bridge) bridge.toggleJulia()
                }
            }
            NavTab {
                tabIndex: 5; icon: "tune";      label: "Settings"
                active: currentTab === 2 && !formulasPopupVisible
                onTapped: { formulasPopupVisible = false; currentTab = 2 }
            }
        }
    }

    component IconButton: Rectangle {
        property string icon: ""
        property color  accent: accentCyan
        signal clicked()

        width: root.isWide ? 40 : 36
        height: root.isWide ? 40 : 36
        radius: 11
        color: ibArea.pressed ? Theme.alpha(accent, 0.15)
                              : ibArea.containsMouse ? Theme.alpha(accent, 0.11)
                                                     : Theme.alpha(accent, 0.07)
        border.color: ibArea.pressed ? Theme.alpha(accent, 0.40)
                                     : ibArea.containsMouse ? Theme.alpha(accent, 0.28)
                                                            : Theme.alpha(accent, 0.15)
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: parent.icon
            font.family: Theme.iconFont
            font.pixelSize: root.isWide ? 24 : 22
            color: parent.accent
        }

        MouseArea {
            id: ibArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }

        Behavior on color { ColorAnimation { duration: 130 } }
        Behavior on border.color { ColorAnimation { duration: 130 } }
    }

    component StatPill: Rectangle {
        property string label: ""
        property string value: "—"

        height: root.isWide ? 27 : 24
        radius: 8
        width: row.implicitWidth + 20
        color: Theme.alpha(Theme.bgDark, 0.72)
        border.color: borderSubtle; border.width: 1

        Row {
            id: row
            anchors.centerIn: parent
            spacing: 7

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: parent.parent.label
                font.pixelSize: root.isWide ? 10 : 9
                font.bold: true
                font.letterSpacing: 2
                color: accentCyan
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: parent.parent.value
                font.pixelSize: root.isWide ? 11 : 10
                font.family: "monospace"
                color: textPrimary
            }
        }
    }

    component ZoomFab: Rectangle {
        property string icon: ""
        property bool   holdable: false
        property bool   active: false
        signal action()
        signal release()

        width: root.isWide ? 46 : 40
        height: root.isWide ? 46 : 40
        radius: 13
        color: zfArea.pressed
               ? Theme.alpha(Theme.accentCyan, 0.13)
               : active
                 ? Theme.alpha(Theme.accentCyan, 0.16)
                 : zfArea.containsMouse
                   ? Theme.alpha(Theme.accentCyan, 0.10)
                   : Theme.alpha(Theme.bgDark, 0.70)
        border.color: (zfArea.pressed || active) ? accentCyan
                                                 : zfArea.containsMouse ? Theme.alpha(Theme.accentCyan, 0.45)
                                                                        : borderBright
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: parent.icon
            font.family: Theme.iconFont
            font.pixelSize: root.isWide ? 27 : 24
            color: (zfArea.pressed || parent.active) ? accentCyan : textPrimary
        }

        MouseArea {
            id: zfArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onPressed: { if (parent.holdable) parent.action() }
            onReleased: { if (parent.holdable) parent.release() }
            onClicked: { if (!parent.holdable) parent.action() }
        }

        Behavior on color       { ColorAnimation { duration: 110 } }
        Behavior on border.color { ColorAnimation { duration: 110 } }
    }

    component StatusBadge: Rectangle {
        id: badge
        property color  dotColor:    accentCyan
        property string labelText:   ""
        property color  badgeColor:  "transparent"
        property color  badgeBorder: borderSubtle
        property bool   pulseDot:    false

        height: root.isWide ? 25 : 22
        radius: 8
        width: badgeRow.implicitWidth + 18
        color: badgeColor
        border.color: badgeBorder; border.width: 1

        Row {
            id: badgeRow
            anchors.centerIn: parent
            spacing: 5

            Rectangle {
                id: dot
                width: 6; height: 6; radius: 3
                color: badge.dotColor
                anchors.verticalCenter: parent.verticalCenter

                // Referenced through the component id: inside an Animation,
                // `parent` is not the visual parent, so the old
                // parent.parent.pulseDot binding was always undefined and the
                // dot never pulsed.
                SequentialAnimation on opacity {
                    running: badge.pulseDot
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 700; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: badge.labelText
                font.pixelSize: root.isWide ? 10 : 9
                font.bold: true
                font.letterSpacing: 2
                color: badge.dotColor
            }
        }
    }

    component NavTab: Item {
        property int    tabIndex: 0
        property string icon:     ""
        property string label:    ""
        property bool   active:   false
        signal tapped()

        // Width comes from the capped nav row, not the whole window.
        width: bottomNav.tabRowWidth / 6
        height: bottomNav.height

        Rectangle {
            anchors.fill: parent
            anchors.margins: 3
            radius: Theme.radiusSm
            color: navArea.containsMouse && !parent.active
                   ? Theme.alpha(Theme.textPrimary, 0.05) : "transparent"
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: 22; height: 2; radius: 1
            color: accentCyan
            visible: parent.active
        }

        Column {
            anchors.centerIn: parent
            spacing: 3

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: parent.parent.icon
                font.family: Theme.iconFont
                font.pixelSize: root.isWide ? 25 : 23
                color: parent.parent.active ? accentCyan
                                            : navArea.containsMouse ? textPrimary : textSecondary
                Behavior on color { ColorAnimation { duration: 180 } }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: parent.parent.label
                font.pixelSize: root.isWide ? 10 : 9
                font.letterSpacing: 0.5
                font.weight: Font.Medium
                color: parent.parent.active ? accentCyan
                                            : navArea.containsMouse ? textSecondary : textDim
                Behavior on color { ColorAnimation { duration: 180 } }
            }
        }

        MouseArea {
            id: navArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.tapped()
        }
    }

    component StyledToggle: Rectangle {
        property bool checked: false
        signal toggled()

        width: 42; height: 24; radius: 12
        color: checked
               ? Theme.alpha(Theme.accentCyan, 0.30)
               : Qt.rgba(1, 1, 1, 0.07)
        border.color: checked ? accentCyan : borderBright; border.width: 1

        Rectangle {
            x: parent.checked ? (parent.width - width - 4) : 4
            y: (parent.height - height) / 2
            width: 16; height: 16; radius: 8
            color: parent.checked ? accentCyan : textDim

            Behavior on x     { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
            Behavior on color { ColorAnimation  { duration: 160 } }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: parent.toggled()
        }

        Behavior on color       { ColorAnimation { duration: 160 } }
        Behavior on border.color { ColorAnimation { duration: 160 } }
    }

    component SettingsToggleRow: Item {
        property string icon:          ""
        property string label:         ""
        property string subtitle:      ""
        property bool   subtitleActive: false
        property color  iconColor:     accentCyan
        property color  iconBg:        Theme.alpha(Theme.accentCyan, 0.10)
        property color  iconBorder:    Theme.alpha(Theme.accentCyan, 0.18)
        property bool   checked:       false
        property bool   showDivider:   true
        signal toggled()

        height: 54

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            height: 1; color: borderSubtle
            visible: parent.showDivider
        }

        Row {
            anchors.fill: parent
            spacing: 12

            IconBadge {
                anchors.verticalCenter: parent.verticalCenter
                icon: parent.parent.icon
                iconColor: parent.parent.iconColor
                bgColor: parent.parent.iconBg
                borderColor: parent.parent.iconBorder
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                width: parent.width - 34 - 12 - 50 - 12

                Text {
                    text: parent.parent.parent.label
                    font.pixelSize: 13; font.weight: Font.Medium
                    color: textPrimary
                }
                Text {
                    text: parent.parent.parent.subtitle
                    font.pixelSize: 10
                    color: parent.parent.parent.subtitleActive ? accentCyan : textDim
                }
            }

            Item { width: parent.width - 34 - 12 - (parent.width - 34 - 12 - 50 - 12) - 12 - 42 - 12; height: 1 }

            StyledToggle {
                anchors.verticalCenter: parent.verticalCenter
                checked: parent.parent.checked
                onToggled: parent.parent.toggled()
            }
        }
    }

    component SettingsActionRow: Item {
        property string icon:        ""
        property string label:       ""
        property string subtitle:    ""
        property color  iconColor:   accentCyan
        property color  iconBg:      Theme.alpha(Theme.accentCyan, 0.10)
        property color  iconBorder:  Theme.alpha(Theme.accentCyan, 0.18)
        property bool   showDivider: true
        signal tapped()

        height: 54

        Rectangle {
            anchors.fill: parent
            color: arArea.pressed ? Theme.alpha(Theme.accentCyan, 0.09)
                                  : arArea.containsMouse ? Theme.alpha(Theme.accentCyan, 0.05)
                                                         : "transparent"
            Behavior on color { ColorAnimation { duration: 100 } }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            height: 1; color: borderSubtle
            visible: parent.showDivider
        }

        Row {
            anchors.fill: parent
            spacing: 12

            IconBadge {
                anchors.verticalCenter: parent.verticalCenter
                icon: parent.parent.icon
                iconColor: parent.parent.iconColor
                bgColor: parent.parent.iconBg
                borderColor: parent.parent.iconBorder
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                width: parent.width - 34 - 12 - 22 - 24
                Text {
                    text: parent.parent.parent.label
                    font.pixelSize: 13; font.weight: Font.Medium
                    color: textPrimary
                }
                Text {
                    text: parent.parent.parent.subtitle
                    font.pixelSize: 10
                    color: textDim
                }
            }

            Item {
                width: parent.width - 34 - 12 - (parent.width - 34 - 12 - 22 - 24) - 12 - 22 - 12
                height: 1
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "chevron_right"
                font.family: Theme.iconFont
                font.pixelSize: 22
                color: textDim
            }
        }

        MouseArea {
            id: arArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.tapped()
        }
    }

    component HistoryBtn: Rectangle {
        property string icon:        ""
        property string label:       ""
        property color  iconColor:   accentCyan
        property color  iconBg:      Theme.alpha(Theme.accentCyan, 0.10)
        property color  iconBorder:  Theme.alpha(Theme.accentCyan, 0.18)
        signal tapped()

        height: 56; radius: 13
        color: hArea.pressed ? Theme.alpha(Theme.accentCyan, 0.06) : bgCard
        border.color: hArea.pressed ? Theme.alpha(Theme.accentCyan, 0.25) : borderSubtle
        border.width: 1

        Column {
            anchors.centerIn: parent
            spacing: 6

            IconBadge {
                anchors.horizontalCenter: parent.horizontalCenter
                icon: parent.parent.icon
                iconColor: parent.parent.iconColor
                bgColor: parent.parent.iconBg
                borderColor: parent.parent.iconBorder
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: parent.parent.label
                font.pixelSize: 11; font.weight: Font.Medium
                color: textSecondary
            }
        }

        MouseArea {
            id: hArea
            anchors.fill: parent
            onClicked: parent.tapped()
        }

        Behavior on color       { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    ShareDialog {
        id: shareDialog
    }

    FractalDetail {
        id: fractalDetailPopup
    }

}
