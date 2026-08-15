import QtQuick
import "."

/*
 * StatChip — a Material glyph paired with a value, for card metadata such as
 * likes and download counts.
 */
Row {
    id: chip

    property string glyph: ""
    property string value: ""
    property color  accent: Theme.textDim

    spacing: Theme.s1

    Text {
        anchors.verticalCenter: parent.verticalCenter
        text: chip.glyph
        font.family: Theme.iconFont
        font.pixelSize: Theme.fontMd
        color: chip.accent
    }

    Text {
        anchors.verticalCenter: parent.verticalCenter
        text: chip.value
        font.pixelSize: Theme.fontSm
        font.weight: Font.Medium
        color: chip.accent
    }
}
