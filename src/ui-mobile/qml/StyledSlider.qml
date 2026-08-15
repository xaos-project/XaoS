import QtQuick
import QtQuick.Controls.Basic
import "."

/*
 * StyledSlider — cyan-to-purple filled track with a ringed handle.
 */
Slider {
    id: control

    implicitWidth: 180
    implicitHeight: 28
    hoverEnabled: true

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: 3
        radius: 1.5
        color: Theme.alpha(Theme.textPrimary, 0.08)

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 1.5
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.accentCyan }
                GradientStop { position: 1.0; color: Theme.accentPurple }
            }
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: 20
        height: 20
        radius: 10
        color: control.pressed ? Theme.accentCyan : Theme.textPrimary
        border.color: Theme.accentCyan
        border.width: 2
        scale: control.hovered && !control.pressed ? 1.1 : 1.0

        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Behavior on scale { NumberAnimation { duration: Theme.durFast } }
    }

    // Cursor feedback on desktop without intercepting the slider's own input.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        cursorShape: Qt.PointingHandCursor
    }
}
