import QtQuick

// GlassButton: A circular glassmorphism button with Material icon support.
// Usage: GlassButton { size: 44; iconCode: "\ue145"; onClicked: ... }

Rectangle {
    id: root

    property int size: 44
    property string iconCode: ""      // Material Symbols codepoint, e.g. "\ue145" for "add"
    property string iconColor: "#CBD5E1" // slate-300
    property int iconSize: 24
    property bool highlight: false

    signal clicked()
    signal pressed()
    signal released()

    width: size
    height: size
    radius: size / 2
    color: highlight ? "#E13535" : Qt.rgba(0.102, 0.102, 0.102, 0.7)
    border.width: highlight ? 0 : 1
    border.color: Qt.rgba(1, 1, 1, 0.08)

    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
    Behavior on color { ColorAnimation { duration: 150 } }

    // Material Symbols font loader
    FontLoader {
        id: materialFont
        source: "qrc:/fonts/MaterialSymbolsOutlined.ttf"
    }

    Text {
        anchors.centerIn: parent
        text: root.iconCode
        color: highlight ? "white" : root.iconColor
        font.pixelSize: root.iconSize
        font.family: materialFont.name
    }

    MouseArea {
        anchors.fill: parent
        anchors.margins: -4
        onPressed: { root.scale = 0.9; root.pressed(); }
        onReleased: { root.scale = 1.0; root.released(); root.clicked(); }
        onCanceled: { root.scale = 1.0; root.released(); }
    }
}
