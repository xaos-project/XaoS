import QtQuick
import "."

/*
 * SegmentedControl — one track with a sliding thumb, replacing the ad-hoc
 * pairs of full-width buttons previously used for Public/Private,
 * Recent/Popular and Login/Sign Up.
 */
Item {
    id: control

    property var    options:      []   // ["Public", "Private"]
    property var    glyphs:       []   // optional Material glyph per option
    property int    currentIndex: 0
    property color  accent:       Theme.accentCyan

    signal activated(int index)

    implicitWidth: 260
    implicitHeight: 42

    Rectangle {
        id: track
        anchors.fill: parent
        radius: Theme.radiusMd
        color: Theme.bgCard
        border.color: Theme.borderSubtle
        border.width: 1
        clip: true

        readonly property real segmentWidth:
            control.options.length > 0 ? (width - 8) / control.options.length : width

        Rectangle {
            id: thumb
            y: 4
            x: 4 + control.currentIndex * track.segmentWidth
            width: track.segmentWidth
            height: parent.height - 8
            radius: Theme.radiusSm
            color: Theme.alpha(control.accent, 0.16)
            border.color: Theme.alpha(control.accent, 0.45)
            border.width: 1

            Behavior on x { NumberAnimation { duration: Theme.durBase; easing.type: Easing.OutCubic } }
            Behavior on color { ColorAnimation { duration: Theme.durBase } }
        }

        Row {
            anchors.fill: parent
            anchors.margins: 4

            Repeater {
                model: control.options

                delegate: Item {
                    id: segment
                    width: track.segmentWidth
                    height: parent.height

                    readonly property bool selected: index === control.currentIndex

                    Row {
                        anchors.centerIn: parent
                        spacing: Theme.s1 + 2

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: index < control.glyphs.length
                            text: index < control.glyphs.length ? control.glyphs[index] : ""
                            font.family: Theme.iconFont
                            font.pixelSize: Theme.fontLg
                            color: segment.selected ? control.accent
                                                    : segmentArea.containsMouse ? Theme.textSecondary
                                                                                : Theme.textDim
                            Behavior on color { ColorAnimation { duration: Theme.durFast } }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData
                            font.pixelSize: Theme.fontBody
                            font.weight: segment.selected ? Font.DemiBold : Font.Medium
                            color: segment.selected ? control.accent
                                                    : segmentArea.containsMouse ? Theme.textPrimary
                                                                                : Theme.textSecondary
                            Behavior on color { ColorAnimation { duration: Theme.durFast } }
                        }
                    }

                    MouseArea {
                        id: segmentArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            control.currentIndex = index
                            control.activated(index)
                        }
                    }
                }
            }
        }
    }
}
