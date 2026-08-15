import QtQuick
import QtQuick.Controls.Basic
import "."

/*
 * GhostButton — tinted, outlined secondary action. Matches the "Randomise
 * Palette" and "Apply Formula" treatment already used in main.qml.
 */
Button {
    id: control

    property color  accent:    Theme.accentCyan
    property string iconGlyph: ""
    property bool   compact:   false

    implicitHeight: compact ? 34 : 46
    implicitWidth: Math.max(compact ? 80 : 120,
                            contentRow.implicitWidth + (compact ? Theme.s4 : Theme.s6) * 2)
    hoverEnabled: true
    padding: 0

    background: Rectangle {
        radius: control.compact ? Theme.radiusSm : Theme.radiusLg
        color: !control.enabled ? "transparent"
                                : control.pressed ? Theme.alpha(control.accent, 0.22)
                                                  : control.hovered ? Theme.alpha(control.accent, 0.13)
                                                                    : Theme.alpha(control.accent, 0.07)
        border.color: !control.enabled ? Theme.borderSubtle
                                       : (control.pressed || control.hovered)
                                         ? Theme.alpha(control.accent, 0.45)
                                         : Theme.alpha(control.accent, 0.20)
        border.width: 1

        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }

    contentItem: Item {
        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: control.compact ? Theme.s1 + 2 : Theme.s2

            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.iconGlyph.length > 0
                text: control.iconGlyph
                font.family: Theme.iconFont
                font.pixelSize: control.compact ? Theme.fontLg : Theme.fontXl
                color: control.enabled ? control.accent : Theme.textDim
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font.pixelSize: control.compact ? Theme.fontSm : Theme.fontBody
                font.weight: Font.DemiBold
                color: control.enabled ? control.accent : Theme.textDim
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
