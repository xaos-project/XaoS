import QtQuick
import QtQuick.Layouts
import "."

/*
 * SectionLabel — letterspaced group heading with a hairline rule beneath,
 * used to break long scrolling panels into sections.
 */
Item {
    id: section

    property string text: ""
    property color  accent: Theme.accentCyan

    Layout.fillWidth: true
    Layout.preferredHeight: 38
    Layout.leftMargin: Theme.s4
    Layout.rightMargin: Theme.s4
    Layout.topMargin: 14

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.s2
        text: section.text
        font.pixelSize: Theme.fontXs
        font.letterSpacing: Theme.trackingWide
        font.weight: Font.DemiBold
        color: section.accent
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.borderSubtle
    }
}
