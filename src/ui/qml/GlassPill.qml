import QtQuick

// GlassPill: A rounded-pill glassmorphism label/badge.
// Usage: GlassPill { labelText: "DEEP ZOOM 10⁴" }

Rectangle {
    id: root

    property string labelText: ""
    property int textSize: 10
    property string textColor: "white"
    property bool isMono: false
    property bool isBold: true
    property bool isUppercase: true

    implicitWidth: content.implicitWidth + 24
    implicitHeight: content.implicitHeight + 12
    radius: height / 2
    color: Qt.rgba(0.102, 0.102, 0.102, 0.7)
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, 0.08)

    Text {
        id: content
        anchors.centerIn: parent
        text: root.isUppercase ? root.labelText.toUpperCase() : root.labelText
        color: root.textColor
        font.pixelSize: root.textSize
        font.weight: root.isBold ? Font.Bold : Font.Normal
        font.family: root.isMono ? "monospace" : "system-ui"
        font.letterSpacing: root.isBold ? 1.0 : 0
        lineHeight: 1.3
    }
}
