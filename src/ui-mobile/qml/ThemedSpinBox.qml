import QtQuick
import QtQuick.Controls.Basic
import "."

/*
 * ThemedSpinBox — dark numeric stepper. The stock Basic.SpinBox renders a
 * light background, which stood out badly against the dark panels.
 */
SpinBox {
    id: control

    property color accent: Theme.accentCyan

    implicitWidth: 140
    implicitHeight: 36
    hoverEnabled: true
    editable: true

    background: Rectangle {
        radius: Theme.radiusSm
        color: Theme.bgSurface
        border.color: control.activeFocus ? control.accent
                                          : control.hovered ? Theme.borderBright
                                                            : Theme.borderSubtle
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }

    contentItem: TextInput {
        text: control.displayText
        font.pixelSize: Theme.fontBody
        font.family: "monospace"
        font.weight: Font.Medium
        color: Theme.textPrimary
        selectionColor: Theme.alpha(control.accent, 0.35)
        selectedTextColor: Theme.textPrimary
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    up.indicator: Rectangle {
        x: control.width - width
        height: control.height
        implicitWidth: 34
        radius: Theme.radiusSm
        color: control.up.pressed ? Theme.alpha(control.accent, 0.22)
                                  : control.up.hovered ? Theme.alpha(control.accent, 0.12)
                                                       : "transparent"

        Behavior on color { ColorAnimation { duration: Theme.durFast } }

        Text {
            anchors.centerIn: parent
            text: "add"
            font.family: Theme.iconFont
            font.pixelSize: Theme.fontLg
            color: control.up.hovered ? control.accent : Theme.textSecondary
        }
    }

    down.indicator: Rectangle {
        height: control.height
        implicitWidth: 34
        radius: Theme.radiusSm
        color: control.down.pressed ? Theme.alpha(control.accent, 0.22)
                                    : control.down.hovered ? Theme.alpha(control.accent, 0.12)
                                                           : "transparent"

        Behavior on color { ColorAnimation { duration: Theme.durFast } }

        Text {
            anchors.centerIn: parent
            text: "remove"
            font.family: Theme.iconFont
            font.pixelSize: Theme.fontLg
            color: control.down.hovered ? control.accent : Theme.textSecondary
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        cursorShape: Qt.PointingHandCursor
    }
}
