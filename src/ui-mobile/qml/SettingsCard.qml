import QtQuick
import QtQuick.Layouts
import "."

/*
 * SettingsCard — padded card that sizes itself to its children.
 */
Rectangle {
    id: card

    default property alias contents: innerPad.data

    Layout.preferredHeight: innerPad.childrenRect.height + 28

    radius: Theme.radiusLg
    color: Theme.bgCard
    border.color: Theme.borderSubtle
    border.width: 1

    Item {
        id: innerPad
        x: 14
        y: 14
        width: parent.width - 28
        height: childrenRect.height
    }
}
