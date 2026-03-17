import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// MobileOverlay.qml — Main explorer screen overlay with Material Symbols icons.
// Matches the "Refined Fractal Explorer" HTML mockup.
// Requires a context property "bridge" (FractalBridge) to be set.

Item {
    id: overlay
    anchors.fill: parent

    // Material Symbols font loader (shared across overlay)
    FontLoader {
        id: materialFont
        source: "qrc:/fonts/MaterialSymbolsOutlined.ttf"
    }

    // ─── Gradient vignette overlays ───────────────────────────
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 160
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: "#0D0D0D" }
        }
        z: 0
    }

    // ─── TOP HEADER ──────────────────────────────────────────
    Item {
        id: topHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: bridge.safeTop + 10  // safe area + padding
        anchors.leftMargin: bridge.safeLeft + 20
        anchors.rightMargin: bridge.safeRight + 20
        height: 60
        z: 10

        // Reset button (top-left) — restart_alt: f053
        GlassButton {
            id: resetBtn
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            size: 40
            iconCode: "\uf053"
            iconSize: 22
            onClicked: bridge.executeCommand("initstate")
        }

        // Right side: zoom badge + coordinates
        Column {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            GlassPill {
                anchors.right: parent.right
                labelText: bridge.zoomLevel
                textSize: 10
                textColor: "white"
                isBold: true
            }

            GlassPill {
                anchors.right: parent.right
                labelText: bridge.realCoord + "\n" + bridge.imagCoord
                textSize: 10
                textColor: "#94A3B8"
                isMono: true
                isBold: false
                isUppercase: false
            }
        }
    }

    // ─── RIGHT-SIDE ZOOM CONTROLS ────────────────────────────
    Column {
        id: zoomControls
        anchors.right: parent.right
        anchors.rightMargin: bridge.safeRight + 20
        anchors.verticalCenter: parent.verticalCenter
        spacing: 16
        z: 10

        GlassButton {
            size: 44
            iconCode: "\ue145"
            iconSize: 24
            onPressed: bridge.startZoomIn()
            onReleased: bridge.stopZoom()
        }

        // remove: e15b
        GlassButton {
            size: 44
            iconCode: "\ue15b"
            iconSize: 24
            onPressed: bridge.startZoomOut()
            onReleased: bridge.stopZoom()
        }


    }

    // ─── BOTTOM TOOLBAR ──────────────────────────────────────
    Item {
        id: bottomToolbar
        anchors.bottom: bottomNav.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 20
        anchors.leftMargin: bridge.safeLeft + 20
        anchors.rightMargin: bridge.safeRight + 20
        height: 56
        z: 10

        // Left: Palette + Share
        Row {
            id: leftButtons
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            // palette: e40a
            GlassButton {
                size: 48
                iconCode: "\ue40a"
                iconSize: 22
                onClicked: bridge.openMenu("palettemenu")
            }

            // share: e80d
            GlassButton {
                size: 48
                iconCode: "\ue80d"
                iconSize: 22
                onClicked: bridge.executeCommand("saveimg")
            }
        }

        // Center: Explore / Animate toggle
        Rectangle {
            id: modeToggle
            anchors.centerIn: parent
            width: 192
            height: 44
            radius: 22
            color: Qt.rgba(0.102, 0.102, 0.102, 0.7)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.08)

            property bool exploreMode: !bridge.autopilotActive

            Row {
                anchors.fill: parent
                anchors.margins: 4

                Rectangle {
                    width: parent.width / 2
                    height: parent.height
                    radius: height / 2
                    color: modeToggle.exploreMode ? "white" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "EXPLORE"
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        font.letterSpacing: 1
                        color: modeToggle.exploreMode ? "black" : "#94A3B8"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (!modeToggle.exploreMode) {
                                bridge.executeCommand("autopilot")
                            }
                        }
                    }

                    Behavior on color { ColorAnimation { duration: 200 } }
                }

                Rectangle {
                    width: parent.width / 2
                    height: parent.height
                    radius: height / 2
                    color: !modeToggle.exploreMode ? "white" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "ANIMATE"
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        font.letterSpacing: 1
                        color: !modeToggle.exploreMode ? "black" : "#94A3B8"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (modeToggle.exploreMode) {
                                bridge.executeCommand("autopilot")
                            }
                        }
                    }

                    Behavior on color { ColorAnimation { duration: 200 } }
                }
            }
        }

        // Right: FAB with expandable menu
        Item {
            id: fabContainer
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 56
            height: 56

            property bool expanded: false

            // Sub-buttons (appear above FAB when expanded)
            Column {
                anchors.bottom: fabBtn.top
                anchors.bottomMargin: 12
                anchors.horizontalCenter: fabBtn.horizontalCenter
                spacing: 12
                opacity: fabContainer.expanded ? 1.0 : 0.0
                visible: opacity > 0
                z: 20

                Behavior on opacity {
                    NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                }

                // settings: e8b8
                GlassButton {
                    size: 44
                    iconCode: "\ue8b8"
                    iconSize: 20
                    onClicked: {
                        fabContainer.expanded = false;
                        settingsSheet.open();
                    }
                }

                // movie: e404
                GlassButton {
                    size: 44
                    iconCode: "\ue404"
                    iconSize: 20
                    onClicked: {
                        fabContainer.expanded = false;
                        bridge.executeCommand("record");
                    }
                }

                // save: e161
                GlassButton {
                    size: 44
                    iconCode: "\ue161"
                    iconSize: 20
                    onClicked: {
                        fabContainer.expanded = false;
                        bridge.executeCommand("savepos");
                    }
                }
            }

            // Main FAB button — add: e145 / close: e5cd
            GlassButton {
                id: fabBtn
                anchors.centerIn: parent
                size: 56
                iconCode: fabContainer.expanded ? "\ue5cd" : "\ue145"
                iconSize: 28
                iconColor: "white"
                highlight: true
                onClicked: fabContainer.expanded = !fabContainer.expanded
            }
        }
    }

    // ─── WATERMARK ───────────────────────────────────────────
    Text {
        text: "XaoS"
        anchors.right: parent.right
        anchors.bottom: bottomToolbar.top
        anchors.rightMargin: bridge.safeRight + 20
        anchors.bottomMargin: 10
        font.pixelSize: 18
        font.weight: Font.Bold
        font.letterSpacing: 2
        color: Qt.rgba(1, 1, 1, 0.3)
        z: 0
    }

    // ─── BOTTOM NAVIGATION BAR ───────────────────────────────
    BottomNavBar {
        id: bottomNav
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        safeBottom: bridge.safeBottom
        z: 10

        onTabClicked: function(index) {
            console.log("Tab clicked:", index);
        }
    }

    // ─── SETTINGS BOTTOM SHEET ───────────────────────────────
    SettingsSheet {
        id: settingsSheet
        anchors.fill: parent
        z: 20
    }

    // ─── GESTURE HANDLING ────────────────────────────────────
    // Capture pinch/pan on the background and forward to C++ bridge
    PinchHandler {
        id: pinchHandler
        target: null // Don't move the overlay itself
        minimumPointCount: 2
        onActiveChanged: if (active) bridge.gesturePinchStarted()
        onActiveScaleChanged: {
            bridge.gesturePinch(activeScale, centroid.position.x, centroid.position.y)
        }
    }

    DragHandler {
        id: panHandler
        target: null
        maximumPointCount: 1
        onTranslationChanged: {
            // Forward deltas to bridge for panning
            bridge.gesturePan(translation.x, translation.y, centroid.position.x, centroid.position.y)
        }
        onActiveChanged: {
            if (!active) bridge.gesturePanFinished()
        }
    }
}
