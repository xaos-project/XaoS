import QtQuick
import "."

/*
 * IconBadge — a tinted rounded square holding one Material icon glyph.
 */
Rectangle {
    id: badge

    property string icon:        ""
    property color  iconColor:   Theme.accentCyan
    property color  bgColor:     Theme.alpha(Theme.accentCyan, 0.08)
    property color  borderColor: Theme.alpha(Theme.accentCyan, 0.18)
    property int    size:        34

    width: size
    height: size
    radius: size / 3
    color: bgColor
    border.color: borderColor
    border.width: 1

    Text {
        anchors.centerIn: parent
        text: badge.icon
        font.family: Theme.iconFont
        font.pixelSize: badge.size * 0.62
        color: badge.iconColor
    }
}
