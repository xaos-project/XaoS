import QtQuick
import QtQuick.Controls.Basic
import "."

/*
 * ThemedField — dark text input with an optional eyebrow label and a cyan
 * focus ring. Replaces the white-background fields that previously sat inside
 * the dark Community dialogs.
 */
Item {
    id: root

    property string label:           ""
    property alias  text:            field.text
    property alias  placeholderText: field.placeholderText
    property alias  echoMode:        field.echoMode
    property alias  maximumLength:   field.maximumLength
    property alias  inputMethodHints: field.inputMethodHints
    property alias  readOnly:        field.readOnly
    property alias  inputFocus:      field.activeFocus
    property bool   mono:            false
    property bool   uppercase:       false
    property color  accent:          Theme.accentCyan

    signal accepted()

    implicitWidth: 220
    implicitHeight: (label.length > 0 ? labelText.implicitHeight + Theme.s1 : 0) + 44

    function forceInputFocus() { field.forceActiveFocus() }

    Text {
        id: labelText
        anchors.top: parent.top
        anchors.left: parent.left
        visible: root.label.length > 0
        text: root.label
        font.pixelSize: Theme.fontEyebrow
        font.bold: true
        font.letterSpacing: Theme.trackingTight
        color: Theme.textDim
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 44
        radius: Theme.radiusMd
        color: Theme.bgSurface
        border.color: field.activeFocus ? root.accent
                                        : hoverArea.containsMouse ? Theme.borderBright
                                                                  : Theme.borderSubtle
        border.width: 1

        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }

        MouseArea {
            id: hoverArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
            cursorShape: Qt.IBeamCursor
        }

        TextField {
            id: field
            anchors.fill: parent
            leftPadding: Theme.s3
            rightPadding: Theme.s3
            verticalAlignment: Text.AlignVCenter
            color: Theme.textPrimary
            placeholderTextColor: Theme.textDim
            selectionColor: Theme.alpha(root.accent, 0.35)
            selectedTextColor: Theme.textPrimary
            font.pixelSize: Theme.fontMd
            // Empty family keeps the application default.
            font.family: root.mono ? "monospace" : ""
            font.letterSpacing: root.uppercase ? Theme.trackingTight : 0
            font.capitalization: root.uppercase ? Font.AllUppercase : Font.MixedCase
            background: null
            onAccepted: root.accepted()
        }
    }
}
