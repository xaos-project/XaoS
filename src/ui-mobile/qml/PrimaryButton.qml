import QtQuick
import QtQuick.Controls.Basic
import "."

/*
 * PrimaryButton — filled accent button carrying the main action of a screen.
 * Has four distinct visual tiers: rest, hover, pressed and disabled.
 */
Button {
    id: control

    property color  accent:     Theme.accentCyan
    property string iconGlyph:  ""
    // Dark text reads better than white on every accent in the palette.
    property color  textColor:  Theme.textOnAccent
    // Some buttons are disabled to mark a finished state rather than an
    // unavailable one; those set a fill so they don't read as greyed out.
    property color  disabledFill: Theme.bgCard
    property color  disabledText: Theme.textDim

    implicitHeight: 46
    implicitWidth: Math.max(120, contentRow.implicitWidth + Theme.s6 * 2)
    hoverEnabled: true
    padding: 0

    background: Rectangle {
        radius: Theme.radiusLg
        color: !control.enabled ? control.disabledFill
                                : control.pressed ? Qt.darker(control.accent, 1.25)
                                                  : control.hovered ? Qt.lighter(control.accent, 1.10)
                                                                    : control.accent
        border.color: control.enabled ? "transparent" : Theme.borderSubtle
        border.width: 1

        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }

    contentItem: Item {
        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: Theme.s2

            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.iconGlyph.length > 0
                text: control.iconGlyph
                font.family: Theme.iconFont
                font.pixelSize: Theme.fontXl
                color: control.enabled ? control.textColor : control.disabledText
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font.pixelSize: Theme.fontMd
                font.weight: Font.DemiBold
                color: control.enabled ? control.textColor : control.disabledText
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
