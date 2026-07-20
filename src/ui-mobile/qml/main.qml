import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    //Design tokens
    readonly property color bgDark:        "#0a0e1a"
    readonly property color bgCard:        "#111827"
    readonly property color bgSurface:     "#1a2035"
    readonly property color accentCyan:    "#00d2ff"
    readonly property color accentMagenta: "#e94560"
    readonly property color accentPurple:  "#6c63ff"
    readonly property color accentGreen:   "#00d282"
    readonly property color accentAmber:   "#ffb432"
    readonly property color textPrimary:   "#e8eaf0"
    readonly property color textSecondary: "#8892a4"
    readonly property color textDim:       "#4a5568"
    readonly property color borderSubtle:  "#1e293b"
    readonly property color borderBright:  "#2a3a50"

    readonly property bool isWide: width >= 700
    readonly property real panelWidth: Math.min(width * 0.94, 400)

    // App state
    property int  currentTab:           0      // 0=Explore 1=Palette 2=Settings
    property bool juliaActive:          false
    property bool formulasPopupVisible: false  // Formulas is now a popup/bottom-sheet over Explore

    //Palette preview helpers
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
        // Deterministic pseudo-random base hue derived from the seed
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

    FontLoader { id: materialFont; source: "qrc:/fonts/MaterialIcons-Regular.ttf" }



    // TOUCH GESTURE AREA
    MultiPointTouchArea {
        id: touchArea
        anchors.fill: parent
        z: 0
        mouseEnabled: true
        minimumTouchPoints: 1
        maximumTouchPoints: 2
        // Stay interactive on the Palette tab too — the palette is now a
        // panel, so the visible fractal area can still be panned / zoomed
        // while colours update in real time.
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
                bridge.startZoomIn()
                zoomPulseTimer.restart()
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

    // SCREEN STACK
    Item {
        id: screenStack
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomNav.top
        z: 5

        //EXPLORE
        Item {
            id: exploreScreen
            anchors.fill: parent
            visible: currentTab === 0

            // ── Top bar ──
            Item {
                id: topBar
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 52
                z: 20

                // Subtle gradient fade so bar doesn't hard-clip the canvas
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.rgba(0.04, 0.055, 0.1, 0.88) }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }

                // XAOS wordmark — centre
                Text {
                    anchors.centerIn: parent
                    text: "XAOS"
                    font.pixelSize: 15
                    font.bold: true
                    font.letterSpacing: 5
                    color: textPrimary
                }

                // Community + Share buttons — right side of top bar
                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    IconButton {
                        icon: "group"
                        accent: accentPurple
                        onClicked: communityGallery.open()
                    }

                    IconButton {
                        icon: "share"
                        accent: accentGreen
                        onClicked: shareDialog.open()
                    }
                }
            }

            //Stats overlay — bottom-right, three pills
            Column {
                id: statsOverlay
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.bottomMargin: 10
                spacing: 4
                z: 15

                StatPill {
                    label: "FRAC"; value: bridge ? bridge.formulaName : "—"
                    MouseArea { anchors.fill: parent; onClicked: formulasPopupVisible = true }
                }
                StatPill { label: "ITER"; value: bridge ? bridge.maxIterations.toString() : "—" }
                StatPill { label: "ZOOM"; value: bridge ? bridge.zoomLevel : "1.00×" }
            }

            // Zoom controls — right side
            Column {
                id: zoomControls
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 12
                spacing: 8
                z: 15

                // Autopilot toggle
                ZoomFab {
                    icon: "play_circle"
                    holdable: false
                    active: bridge ? bridge.autopilotActive : false
                    onAction: { if (bridge) bridge.toggleAutopilot() }
                }

                // Thin separator between autopilot and zoom
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

                // Thin separator between zoom and reset
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

            // Status badges — above bottom nav
            Row {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                anchors.left: parent.left
                anchors.leftMargin: 12
                spacing: 8
                z: 15

                // Autopilot badge
                StatusBadge {
                    visible: bridge ? bridge.autopilotActive : false
                    dotColor: accentCyan
                    labelText: "AUTOPILOT"
                    badgeColor: Qt.rgba(0, 0.82, 1, 0.10)
                    badgeBorder: Qt.rgba(0, 0.82, 1, 0.28)
                    pulseDot: true
                }

                // Julia badge
                StatusBadge {
                    visible: root.juliaActive
                    dotColor: accentMagenta
                    labelText: "JULIA"
                    badgeColor: Qt.rgba(0.91, 0.27, 0.38, 0.10)
                    badgeBorder: Qt.rgba(0.91, 0.27, 0.38, 0.28)
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

            // Palette preset data (algorithm, seed combos that produce good palettes)
            ListModel {
                id: palettePresets
                ListElement { palName: "Neon Deep";    alg: 1; seed: 12345; c0: "#0a0e1a"; c1: "#00d2ff"; c2: "#6c63ff"; c3: "#e94560" }
                ListElement { palName: "Fire & Ice";   alg: 2; seed: 8421;  c0: "#0d0020"; c1: "#4400ff"; c2: "#ff6600"; c3: "#ffdd00" }
                ListElement { palName: "Ocean Depth";  alg: 1; seed: 3500;  c0: "#000d1a"; c1: "#003366"; c2: "#006699"; c3: "#00ccff" }
                ListElement { palName: "Sunset";       alg: 3; seed: 19000; c0: "#1a0000"; c1: "#660000"; c2: "#ff4400"; c3: "#ffaa00" }
                ListElement { palName: "Aurora";       alg: 2; seed: 25000; c0: "#000a10"; c1: "#006644"; c2: "#00cc88"; c3: "#ffff66" }
                ListElement { palName: "Ultra Violet"; alg: 3; seed: 7777;  c0: "#0a0010"; c1: "#330044"; c2: "#9900cc"; c3: "#ff66ff" }
                ListElement { palName: "Monochrome";   alg: 1; seed: 42;    c0: "#000000"; c1: "#333333"; c2: "#888888"; c3: "#ffffff" }
                ListElement { palName: "Toxic";        alg: 2; seed: 15000; c0: "#001a00"; c1: "#006600"; c2: "#33ff00"; c3: "#ccff00" }
            }

            // Track which preset is selected (-1 = custom)
            property int selectedPreset: -1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                ScreenHeader {
                    subtitle: "COLOR PALETTE"
                    title: "Palette"
                    showCount: false
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
                            color: Qt.rgba(0, 0.82, 1, 0.25)
                        }
                    }

                    ColumnLayout {
                        id: palCol
                        width: parent.width
                        spacing: 0

                        // PRESET PALETTES
                        SectionLabel { text: "PRESETS" }

                        // 2-column grid
                        GridView {
                            id: palGrid
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            Layout.preferredHeight: Math.ceil(palettePresets.count / 2) * 106
                            clip: true
                            interactive: false
                            cellWidth: (width - 10) / 2
                            cellHeight: 106
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
                                                  ? accentCyan : borderSubtle
                                    border.width: paletteScreen.selectedPreset === index ? 2 : 1
                                    clip: true

                                    // Colour swatch
                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.left: parent.left; anchors.right: parent.right
                                        height: 64; radius: 13
                                        clip: true
                                        
                                        Row {
                                            anchors.fill: parent
                                            spacing: 0
                                            Repeater {
                                                model: 32
                                                Rectangle {
                                                    width: parent.width / 32
                                                    height: parent.height
                                                    color: root.generatePaletteColor(
                                                        presetDelegate.presetAlg,
                                                        presetDelegate.presetSeed,
                                                        0,
                                                        index, 32)
                                                }
                                            }
                                        }
                                    }

                                    // Name strip
                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        anchors.left: parent.left; anchors.right: parent.right
                                        height: 32
                                        color: bgCard

                                        Text {
                                            anchors.left: parent.left; anchors.leftMargin: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: model.palName
                                            font.pixelSize: 11; font.weight: Font.Medium
                                            color: paletteScreen.selectedPreset === index ? accentCyan : textSecondary
                                            Behavior on color { ColorAnimation { duration: 180 } }
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            paletteScreen.selectedPreset = index
                                            // Keep the Custom Palette sliders in sync with
                                            // the preset that was just applied — previously
                                            // they kept showing stale values once a slider
                                            // had been dragged at least once.
                                            algSlider.value = model.alg
                                            seedSlider.value = model.seed
                                            shiftSlider.value = 0
                                            if (bridge) bridge.setCustomPalette(model.alg, model.seed, 0)
                                        }
                                    }

                                    Behavior on border.color { ColorAnimation { duration: 180 } }
                                }
                            }
                        }

                        // Sync sliders when engine palette changes (e.g. Randomise)
                        Connections {
                            target: bridge
                            function onStateChanged() {
                                if (algSlider && seedSlider && shiftSlider) {
                                    if (algSlider.value !== bridge.paletteAlgorithm)
                                        algSlider.value = bridge.paletteAlgorithm;
                                    if (seedSlider.value !== bridge.paletteSeed)
                                        seedSlider.value = bridge.paletteSeed;
                                    if (shiftSlider.value !== bridge.paletteShift)
                                        shiftSlider.value = bridge.paletteShift;
                                }
                            }
                        }

                        // CUSTOM PALETTE
                        SectionLabel { text: "CUSTOM PALETTE" }

                        // Algorithm slider
                        SettingsCard {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16

                            Column {
                                width: parent.width
                                spacing: 10

                                Row {
                                    width: parent.width
                                    Text {
                                        text: "Algorithm"
                                        font.pixelSize: 13; font.weight: Font.Medium
                                        color: textPrimary
                                        width: parent.width - algValText.implicitWidth
                                    }
                                    Text {
                                        id: algValText
                                        text: algSlider.value.toFixed(0)
                                        font.pixelSize: 12; font.bold: true
                                        font.family: "monospace"
                                        color: accentMagenta
                                    }
                                }

                                StyledSlider {
                                    id: algSlider
                                    width: parent.width
                                    from: 1; to: 3; stepSize: 1
                                    value: bridge ? bridge.paletteAlgorithm : 1
                                    onMoved: {
                                        paletteScreen.selectedPreset = -1
                                        if (bridge) bridge.setCustomPalette(
                                            Math.round(value),
                                            Math.round(seedSlider.value),
                                            Math.round(shiftSlider.value))
                                    }
                                }
                            }
                        }

                        // Seed slider
                        SettingsCard {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            Layout.topMargin: 8

                            Column {
                                width: parent.width
                                spacing: 10

                                Row {
                                    width: parent.width
                                    Text {
                                        text: "Seed"
                                        font.pixelSize: 13; font.weight: Font.Medium
                                        color: textPrimary
                                        width: parent.width - seedValText.implicitWidth
                                    }
                                    Text {
                                        id: seedValText
                                        text: seedSlider.value.toFixed(0)
                                        font.pixelSize: 12; font.bold: true
                                        font.family: "monospace"
                                        color: accentMagenta
                                    }
                                }

                                StyledSlider {
                                    id: seedSlider
                                    width: parent.width
                                    from: 0; to: 65535; stepSize: 1
                                    value: bridge ? bridge.paletteSeed : 0
                                    onMoved: {
                                        paletteScreen.selectedPreset = -1
                                        if (bridge) bridge.setCustomPalette(
                                            Math.round(algSlider.value),
                                            Math.round(value),
                                            Math.round(shiftSlider.value))
                                    }
                                }
                            }
                        }

                        // Shift slider
                        SettingsCard {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            Layout.topMargin: 8

                            Column {
                                width: parent.width
                                spacing: 10

                                Row {
                                    width: parent.width
                                    Text {
                                        text: "Shift"
                                        font.pixelSize: 13; font.weight: Font.Medium
                                        color: textPrimary
                                        width: parent.width - shiftValText.implicitWidth
                                    }
                                    Text {
                                        id: shiftValText
                                        text: shiftSlider.value.toFixed(0)
                                        font.pixelSize: 12; font.bold: true
                                        font.family: "monospace"
                                        color: accentMagenta
                                    }
                                }

                                StyledSlider {
                                    id: shiftSlider
                                    width: parent.width
                                    from: 0; to: 255; stepSize: 1
                                    value: bridge ? bridge.paletteShift : 0
                                    onMoved: {
                                        paletteScreen.selectedPreset = -1
                                        if (bridge) bridge.setCustomPalette(
                                            Math.round(algSlider.value),
                                            Math.round(seedSlider.value),
                                            Math.round(value))
                                    }
                                }
                            }
                        }

                        Item { Layout.preferredHeight: 14 }

                        // Live preview — reacts instantly to Algorithm / Seed / Shift
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
                                    model: 32
                                    Rectangle {
                                        width: parent.width / 32
                                        height: parent.height
                                        color: root.generatePaletteColor(
                                            Math.round(algSlider.value),
                                            Math.round(seedSlider.value),
                                            Math.round(shiftSlider.value),
                                            index, 32)
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

                        // Randomise button
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16; Layout.rightMargin: 16
                            Layout.bottomMargin: 12
                            height: 48; radius: 13
                            color: Qt.rgba(0, 0.82, 1, 0.07)
                            border.color: Qt.rgba(0, 0.82, 1, 0.22); border.width: 1

                            Row {
                                anchors.centerIn: parent
                                spacing: 8
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "casino"
                                    font.family: materialFont.name
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
                                anchors.fill: parent
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

        //2: SETTINGS
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

                // Header with accent bar
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52

                    // Rainbow accent line at top
                    Rectangle {
                        anchors.top: parent.top
                        anchors.left: parent.left; anchors.right: parent.right
                        height: 2
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: accentCyan }
                            GradientStop { position: 0.5; color: accentPurple }
                            GradientStop { position: 1.0; color: accentMagenta }
                        }
                    }

                    Row {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.bottom: parent.bottom; anchors.bottomMargin: 10
                        spacing: 0

                        Column {
                            spacing: 2
                            Text {
                                text: "XAOS"
                                font.pixelSize: 9; font.bold: true
                                font.letterSpacing: 3
                                color: accentCyan
                            }
                            Text {
                                text: "Settings"
                                font.pixelSize: 18; font.weight: Font.DemiBold
                                color: textPrimary
                            }
                        }
                    }

                    // Bottom border
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left; anchors.right: parent.right
                        height: 1; color: borderSubtle
                    }
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
                            color: Qt.rgba(0, 0.82, 1, 0.25)
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
                        
                            // Left: Current Fractal
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
                            
                            // Right: About / Version
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
                                    text: "v4.3.5"
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

                        // RENDERING
                        SectionLabel { text: "RENDERING & COMPUTE" }

                        // Iteration depth
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



                        //  NAVIGATION
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
                                    iconBg: Qt.rgba(1, 0.71, 0.2, 0.10)
                                    iconBorder: Qt.rgba(1, 0.71, 0.2, 0.18)
                                    showDivider: true
                                    onTapped: { if (bridge) bridge.loadRandomExample() }
                                }

                                SettingsActionRow {
                                    width: parent.width
                                    icon: "home"
                                    label: "Reset View"
                                    subtitle: "Return to default position"
                                    iconColor: accentCyan
                                    iconBg: Qt.rgba(0, 0.82, 1, 0.10)
                                    iconBorder: Qt.rgba(0, 0.82, 1, 0.18)
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
    }

    // FORMULAS POPUP
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

        // Dimmed backdrop — tap to dismiss
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.60)

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

                // Screen header
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

                // Search bar
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
                        font.family: materialFont.name
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

                        // Qt6-compatible placeholder
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

                // User Formula Card 
                Rectangle {
                    id: userFormulaCard
                    Layout.fillWidth: true
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    Layout.topMargin: 6
                    // Collapse to just the header when closed
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
                        // Dismiss the soft keyboard so the fractal is visible
                        Qt.inputMethod.hide()
                    }

                    Column {
                        id: userFormulaCol
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 0
                        spacing: 0

                        // Header row — always visible, acts as toggle
                        Rectangle {
                            width: parent.width; height: 40
                            color: "transparent"

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 12
                                spacing: 8

                                Text {
                                    text: "functions"
                                    font.family: materialFont.name; font.pixelSize: 18
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
                                font.family: materialFont.name; font.pixelSize: 20
                                color: textSecondary
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: userFormulaCard.userFormulaExpanded =
                                           !userFormulaCard.userFormulaExpanded
                            }
                        }

                        // ── Current formula display ──
                        Rectangle {
                            width: parent.width - 24; height: 32
                            anchors.horizontalCenter: parent.horizontalCenter
                            radius: 8; color: Qt.rgba(0, 0.82, 1, 0.06)
                            border.color: Qt.rgba(0, 0.82, 1, 0.15); border.width: 1

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

                        // ── Formula input ──
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

                                // Pre-fill with current expression when popup opens
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

                        // Initial value input 
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

                        // ── Apply button ──
                        Rectangle {
                            width: parent.width - 24; height: 38
                            anchors.horizontalCenter: parent.horizontalCenter
                            radius: 10
                            color: applyArea.pressed
                                   ? Qt.rgba(0, 0.82, 1, 0.25)
                                   : Qt.rgba(0, 0.82, 1, 0.12)
                            border.color: accentCyan; border.width: 1

                            Row {
                                anchors.centerIn: parent; spacing: 6
                                Text {
                                    text: "play_arrow"
                                    font.family: materialFont.name; font.pixelSize: 18
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
                                onClicked: userFormulaCard.applyUserFormula()
                            }

                            Behavior on color { ColorAnimation { duration: 120 } }
                        }

                        Item { width: 1; height: 8 }
                    }
                }

                // Formula list
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
                            color: Qt.rgba(0, 0.82, 1, 0.25)
                        }
                    }

                    // Shown when no fractal name matches the search text
                    Text {
                        anchors.centerIn: parent
                        visible: formulaList.count === 0
                        text: "No fractals match your search"
                        font.pixelSize: 13
                        color: textDim
                    }

                    delegate: Rectangle {
                        id: fItem
                        // modelData is the real index into the bridge's formula list
                        property int realIndex: modelData
                        property bool isSelected: bridge ? (bridge.getFormulaName(realIndex) === bridge.formulaName) : false
                        width: formulaList.width
                        height: 52; radius: 11
                        color: fArea.pressed
                               ? Qt.rgba(0, 0.82, 1, 0.10)
                               : isSelected
                                 ? Qt.rgba(0, 0.82, 1, 0.07)
                                 : bgCard
                        border.color: isSelected
                                      ? Qt.rgba(0, 0.82, 1, 0.35)
                                      : fArea.pressed
                                        ? Qt.rgba(0, 0.82, 1, 0.2)
                                        : borderSubtle
                        border.width: 1

                        // Index badge
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

                        // Formula name
                        Text {
                            anchors.left: idxBadge.right; anchors.leftMargin: 10
                            anchors.right: checkMark.left; anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: bridge ? bridge.getFormulaName(fItem.realIndex) : ""
                            font.pixelSize: 14; font.weight: Font.Medium
                            color: fItem.isSelected ? textPrimary : "#ccd0d8"
                            elide: Text.ElideRight
                        }

                        // Check mark for active
                        Text {
                            id: checkMark
                            anchors.right: parent.right; anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter
                            text: "check"
                            font.family: materialFont.name
                            font.pixelSize: 20
                            color: accentCyan
                            visible: fItem.isSelected
                        }

                        MouseArea {
                            id: fArea
                            anchors.fill: parent
                            onClicked: {
                                if (bridge) bridge.setFormula(fItem.realIndex)
                                // Picking a fractal closes the popup automatically
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

    // BOTTOM NAVIGATION BAR  (shared across all screens)
    Rectangle {
        id: bottomNav
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        z: 30
        color: Qt.rgba(0.04, 0.055, 0.1, 0.96)
        border.color: borderSubtle; border.width: 1

        // Glowing top-edge line
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left; anchors.right: parent.right
            height: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.2; color: Qt.rgba(0, 0.82, 1, 0.30) }
                GradientStop { position: 0.5; color: Qt.rgba(0, 0.82, 1, 0.55) }
                GradientStop { position: 0.8; color: Qt.rgba(0, 0.82, 1, 0.30) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Row {
            anchors.fill: parent

            NavTab {
                tabIndex: 0; icon: "explore";  label: "Explore"
                active: currentTab === 0 && !formulasPopupVisible
                onTapped: { formulasPopupVisible = false; currentTab = 0 }
            }
            NavTab {
                tabIndex: 1; icon: "functions"; label: "Formulas"
                active: formulasPopupVisible
                onTapped: {
                    // Formulas is a popup over Explore, not its own screen
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
                tabIndex: 3; icon: "blur_on";   label: "Julia"
                active: root.juliaActive
                onTapped: {
                    // Julia is a direct toggle, not a screen — tap again to turn it off
                    root.juliaActive = !root.juliaActive
                    if (bridge) bridge.toggleJulia()
                }
            }
            NavTab {
                tabIndex: 4; icon: "tune";      label: "Settings"
                active: currentTab === 2 && !formulasPopupVisible
                onTapped: { formulasPopupVisible = false; currentTab = 2 }
            }
        }
    }

    // REUSABLE COMPONENTS

    // Icon button (top bar)
    component IconButton: Rectangle {
        property string icon: ""
        property color  accent: accentCyan
        signal clicked()

        width: 36; height: 36; radius: 11
        color: ibArea.pressed ? Qt.rgba(accent.r, accent.g, accent.b, 0.15) : Qt.rgba(accent.r, accent.g, accent.b, 0.07)
        border.color: ibArea.pressed ? Qt.rgba(accent.r, accent.g, accent.b, 0.40) : Qt.rgba(accent.r, accent.g, accent.b, 0.15)
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: parent.icon
            font.family: materialFont.name
            font.pixelSize: 22
            color: parent.accent
        }

        MouseArea {
            id: ibArea
            anchors.fill: parent
            onClicked: parent.clicked()
        }

        Behavior on color { ColorAnimation { duration: 130 } }
        Behavior on border.color { ColorAnimation { duration: 130 } }
    }

    // Stat pill (Explore overlay)
    component StatPill: Rectangle {
        property string label: ""
        property string value: "—"

        height: 24; radius: 8
        width: row.implicitWidth + 20
        color: Qt.rgba(0.04, 0.055, 0.1, 0.72)
        border.color: borderSubtle; border.width: 1

        Row {
            id: row
            anchors.centerIn: parent
            spacing: 7

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: parent.parent.label
                font.pixelSize: 9; font.bold: true
                font.letterSpacing: 2
                color: accentCyan
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: parent.parent.value
                font.pixelSize: 10; font.family: "monospace"
                color: textPrimary
            }
        }
    }

    // Zoom FAB
    component ZoomFab: Rectangle {
        property string icon: ""
        property bool   holdable: false
        property bool   active: false
        signal action()
        signal release()

        width: 40; height: 40; radius: 13
        color: zfArea.pressed
               ? Qt.rgba(0, 0.82, 1, 0.13)
               : active
                 ? Qt.rgba(0, 0.82, 1, 0.16)
                 : Qt.rgba(0.04, 0.06, 0.1, 0.70)
        border.color: (zfArea.pressed || active) ? accentCyan : borderBright
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: parent.icon
            font.family: materialFont.name
            font.pixelSize: 24
            color: (zfArea.pressed || parent.active) ? accentCyan : textPrimary
        }

        MouseArea {
            id: zfArea
            anchors.fill: parent
            // Holdable buttons (zoom +/-) fire on press and stop on release.
            // Non-holdable buttons (Autopilot, Reset) must fire exactly once
            // per tap — firing on both press AND click double-toggled them.
            onPressed: { if (parent.holdable) parent.action() }
            onReleased: { if (parent.holdable) parent.release() }
            onClicked: { if (!parent.holdable) parent.action() }
        }

        Behavior on color       { ColorAnimation { duration: 110 } }
        Behavior on border.color { ColorAnimation { duration: 110 } }
    }

    // Status badge (Autopilot / Julia)
    component StatusBadge: Rectangle {
        property color  dotColor:    accentCyan
        property string labelText:   ""
        property color  badgeColor:  "transparent"
        property color  badgeBorder: borderSubtle
        property bool   pulseDot:    false

        height: 22; radius: 8
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
                color: parent.parent.dotColor
                anchors.verticalCenter: parent.verticalCenter

                SequentialAnimation on opacity {
                    running: parent.parent.pulseDot
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 700; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: parent.parent.labelText
                font.pixelSize: 9; font.bold: true
                font.letterSpacing: 2
                color: parent.parent.dotColor
            }
        }
    }

    // Nav tab
    component NavTab: Item {
        property int    tabIndex: 0
        property string icon:     ""
        property string label:    ""
        property bool   active:   false
        signal tapped()

        width: root.width / 5
        height: 60

        // Active indicator bar at top
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
                font.family: materialFont.name
                font.pixelSize: 23
                color: parent.parent.active ? accentCyan : textSecondary
                Behavior on color { ColorAnimation { duration: 180 } }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: parent.parent.label
                font.pixelSize: 9; font.letterSpacing: 0.5
                font.weight: Font.Medium
                color: parent.parent.active ? accentCyan : textDim
                Behavior on color { ColorAnimation { duration: 180 } }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: parent.tapped()
        }
    }

    //  Screen header (Formulas / Palette / Julia)
    component ScreenHeader: Item {
        property string subtitle: ""
        property string title: ""
        property string countText: ""
        property bool   showCount: false

        Layout.fillWidth: true
        Layout.preferredHeight: showCount ? 88 : 68

        // Accent bar at very top
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left; anchors.right: parent.right
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: accentCyan }
                GradientStop { position: 0.5; color: accentPurple }
                GradientStop { position: 1.0; color: accentMagenta }
            }
        }

        Column {
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.bottom: parent.bottom; anchors.bottomMargin: 10
            spacing: 4

            Text {
                text: parent.parent.subtitle
                font.pixelSize: 9; font.bold: true; font.letterSpacing: 3
                color: textDim
            }
            Text {
                text: parent.parent.title
                font.pixelSize: 20; font.bold: true
                color: textPrimary
            }

            Rectangle {
                visible: parent.parent.showCount
                height: 22; radius: 11
                width: cntTxt.implicitWidth + 20
                color: Qt.rgba(0, 0.82, 1, 0.08)
                border.color: Qt.rgba(0, 0.82, 1, 0.15); border.width: 1

                Text {
                    id: cntTxt
                    anchors.centerIn: parent
                    text: parent.parent.parent.countText
                    font.pixelSize: 10; font.weight: Font.Medium
                    color: accentCyan
                }
            }
        }

        // Bottom divider
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            height: 1; color: borderSubtle
        }
    }

    //  Section label (inside Settings / Julia)
    component SectionLabel: Item {
        property string text: ""
        Layout.fillWidth: true
        Layout.preferredHeight: 38
        Layout.leftMargin: 16; Layout.rightMargin: 16
        Layout.topMargin: 14

        Text {
            anchors.left: parent.left
            anchors.bottom: parent.bottom; anchors.bottomMargin: 8
            text: parent.text
            font.pixelSize: 10; font.letterSpacing: 3
            font.weight: Font.DemiBold
            color: accentCyan
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            height: 1; color: borderSubtle
        }
    }

    // Settings card wrapper
    component SettingsCard: Rectangle {
        Layout.preferredHeight: innerPad.childrenRect.height + 28
        radius: 13
        color: bgCard
        border.color: borderSubtle; border.width: 1

        default property alias contents: innerPad.data

        Item {
            id: innerPad
            x: 14; y: 14
            width: parent.width - 28
            height: childrenRect.height
        }
    }

    // Icon badge
    component IconBadge: Rectangle {
        property string icon:        ""
        property color  iconColor:   accentCyan
        property color  bgColor:     Qt.rgba(0, 0.82, 1, 0.08)
        property color  borderColor: Qt.rgba(0, 0.82, 1, 0.18)
        property int    size:        34

        width: size; height: size; radius: size / 3
        color: bgColor
        border.color: borderColor; border.width: 1

        Text {
            anchors.centerIn: parent
            text: parent.icon
            font.family: materialFont.name
            font.pixelSize: parent.size * 0.62
            color: parent.iconColor
        }
    }

    // Styled toggle
    component StyledToggle: Rectangle {
        property bool checked: false
        signal toggled()

        width: 42; height: 24; radius: 12
        color: checked
               ? Qt.rgba(0, 0.82, 1, 0.30)
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

    component StyledSlider: Basic.Slider {
        implicitWidth: 180
        implicitHeight: 28

        background: Rectangle {
            x: parent.leftPadding
            y: parent.topPadding + parent.availableHeight / 2 - height / 2
            width: parent.availableWidth; height: 3; radius: 1.5
            color: Qt.rgba(1, 1, 1, 0.08)

            Rectangle {
                width: parent.parent.visualPosition * parent.width
                height: parent.height; radius: 1.5
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: accentCyan }
                    GradientStop { position: 1.0; color: accentPurple }
                }
            }
        }

        handle: Rectangle {
            x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
            y: parent.topPadding + parent.availableHeight / 2 - height / 2
            width: 20; height: 20; radius: 10
            color: parent.pressed ? accentCyan : "#ffffff"
            border.color: accentCyan; border.width: 2

            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }

    // Settings toggle row
    component SettingsToggleRow: Item {
        property string icon:          ""
        property string label:         ""
        property string subtitle:      ""
        property bool   subtitleActive: false
        property color  iconColor:     accentCyan
        property color  iconBg:        Qt.rgba(0, 0.82, 1, 0.10)
        property color  iconBorder:    Qt.rgba(0, 0.82, 1, 0.18)
        property bool   checked:       false
        property bool   showDivider:   true
        signal toggled()

        height: 54

        // Divider
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

            // spacer — explicit width so it works inside Row (not RowLayout)
            Item { width: parent.width - 34 - 12 - (parent.width - 34 - 12 - 50 - 12) - 12 - 42 - 12; height: 1 }

            StyledToggle {
                anchors.verticalCenter: parent.verticalCenter
                checked: parent.parent.checked
                onToggled: parent.parent.toggled()
            }
        }
    }

    // Settings action row
    component SettingsActionRow: Item {
        property string icon:        ""
        property string label:       ""
        property string subtitle:    ""
        property color  iconColor:   accentCyan
        property color  iconBg:      Qt.rgba(0, 0.82, 1, 0.10)
        property color  iconBorder:  Qt.rgba(0, 0.82, 1, 0.18)
        property bool   showDivider: true
        signal tapped()

        height: 54

        Rectangle {
            anchors.fill: parent
            color: arArea.pressed ? Qt.rgba(0, 0.82, 1, 0.05) : "transparent"
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

            // explicit spacer inside Row
            Item {
                width: parent.width - 34 - 12 - (parent.width - 34 - 12 - 22 - 24) - 12 - 22 - 12
                height: 1
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "chevron_right"
                font.family: materialFont.name
                font.pixelSize: 22
                color: textDim
            }
        }

        MouseArea {
            id: arArea
            anchors.fill: parent
            onClicked: parent.tapped()
        }
    }

    // History button (Undo / Redo)
    component HistoryBtn: Rectangle {
        property string icon:        ""
        property string label:       ""
        property color  iconColor:   accentCyan
        property color  iconBg:      Qt.rgba(0, 0.82, 1, 0.10)
        property color  iconBorder:  Qt.rgba(0, 0.82, 1, 0.18)
        signal tapped()

        height: 56; radius: 13
        color: hArea.pressed ? Qt.rgba(0, 0.82, 1, 0.06) : bgCard
        border.color: hArea.pressed ? Qt.rgba(0, 0.82, 1, 0.25) : borderSubtle
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
}
