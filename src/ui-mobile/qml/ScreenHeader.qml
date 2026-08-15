import QtQuick
import QtQuick.Layouts
import "."

/*
 * ScreenHeader — the eyebrow + title block that opens every panel, under a
 * 2px cyan/purple/magenta rule. Children are placed in the trailing slot on
 * the right, e.g. ScreenHeader { GhostButton { text: "Logout" } }.
 */
Item {
    id: header

    property string subtitle: ""
    property string title: ""
    property string countText: ""
    property bool   showCount: false
    property color  countAccent: Theme.accentCyan
    property bool   wide: false
    // Screens whose body is a centred column set this so the title lines up
    // with the content instead of hugging the window edge.
    property real   contentInset: wide ? Theme.s5 : Theme.s4

    default property alias trailing: trailingSlot.data

    Layout.fillWidth: true
    Layout.preferredHeight: (showCount ? 88 : 68) + (wide ? 8 : 0)

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 2
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Theme.accentCyan }
            GradientStop { position: 0.5; color: Theme.accentPurple }
            GradientStop { position: 1.0; color: Theme.accentMagenta }
        }
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: header.contentInset
        anchors.right: trailingSlot.left
        anchors.rightMargin: Theme.s3
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        spacing: Theme.s1

        Text {
            text: header.subtitle
            font.pixelSize: Theme.fontEyebrow
            font.bold: true
            font.letterSpacing: Theme.trackingWide
            color: Theme.textDim
        }

        Text {
            width: parent.width
            text: header.title
            font.pixelSize: header.wide ? Theme.fontXl + 2 : Theme.fontXl
            font.bold: true
            color: Theme.textPrimary
            elide: Text.ElideRight
        }

        Rectangle {
            visible: header.showCount
            height: 22
            radius: 11
            width: countLabel.implicitWidth + Theme.s5
            color: Theme.alpha(header.countAccent, 0.08)
            border.color: Theme.alpha(header.countAccent, 0.15)
            border.width: 1

            Text {
                id: countLabel
                anchors.centerIn: parent
                text: header.countText
                font.pixelSize: Theme.fontXs
                font.weight: Font.Medium
                color: header.countAccent
            }
        }
    }

    Row {
        id: trailingSlot
        anchors.right: parent.right
        anchors.rightMargin: header.contentInset
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        spacing: Theme.s2
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.borderSubtle
    }
}
