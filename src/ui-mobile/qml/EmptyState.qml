import QtQuick
import "."

/*
 * EmptyState — icon, headline and supporting line for empty lists and
 * galleries, replacing bare grey paragraphs.
 */
Item {
    id: state

    property string glyph:    "inbox"
    property string title:    ""
    property string subtitle: ""
    property color  accent:   Theme.accentCyan

    implicitHeight: column.implicitHeight
    implicitWidth: 260

    Column {
        id: column
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.s6 * 2, 320)
        spacing: Theme.s3

        IconBadge {
            anchors.horizontalCenter: parent.horizontalCenter
            icon: state.glyph
            size: 56
            iconColor: state.accent
            bgColor: Theme.alpha(state.accent, 0.07)
            borderColor: Theme.alpha(state.accent, 0.16)
        }

        Text {
            width: parent.width
            text: state.title
            font.pixelSize: Theme.fontMd + 1
            font.weight: Font.DemiBold
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Text {
            width: parent.width
            visible: state.subtitle.length > 0
            text: state.subtitle
            font.pixelSize: Theme.fontSm + 1
            color: Theme.textDim
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            lineHeight: 1.3
        }
    }
}
