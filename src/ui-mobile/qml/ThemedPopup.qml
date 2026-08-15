import QtQuick
import QtQuick.Controls
import "."

/*
 * ThemedPopup — the shared dialog shell: scrim, dark ground, accent rule and
 * a lift-in animation.
 *
 * The popup sizes itself to its content rather than to a fixed height, so
 * subclasses MUST supply their layout as `contentItem:` (a ColumnLayout has a
 * real implicitHeight; a child anchored with `anchors.fill: parent` does not,
 * and would collapse the popup).
 */
Popup {
    id: control

    property int   maxWidth: Theme.maxDialogWidth
    property color accent:   Theme.accentCyan
    property bool  showRule: true

    anchors.centerIn: parent
    width: parent ? Math.min(parent.width * 0.92, maxWidth) : maxWidth
    height: Math.min(implicitHeight, parent ? parent.height * 0.92 : 600)

    padding: Theme.s5
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: Theme.scrim }

    background: Rectangle {
        color: Theme.bgElevated
        radius: Theme.radiusXl
        border.color: Theme.borderBright
        border.width: 1
        clip: true

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 2
            visible: control.showRule
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.accentCyan }
                GradientStop { position: 0.5; color: Theme.accentPurple }
                GradientStop { position: 1.0; color: Theme.accentMagenta }
            }
        }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"; from: 0.0; to: 1.0
                duration: Theme.durBase; easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "scale"; from: 0.96; to: 1.0
                duration: Theme.durSlow; easing.type: Easing.OutCubic
            }
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"; from: 1.0; to: 0.0
            duration: Theme.durFast; easing.type: Easing.InCubic
        }
    }
}
