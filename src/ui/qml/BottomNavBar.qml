import QtQuick

// BottomNavBar: 4-tab navigation bar with Material icons.

Rectangle {
    id: root

    property int currentIndex: 0
    property int safeBottom: 0  // Set from parent using bridge.safeBottom

    signal tabClicked(int index)

    height: 64 + safeBottom  // nav content + home indicator safe area
    color: Qt.rgba(0.051, 0.051, 0.051, 0.9)

    // Top border
    Rectangle {
        width: parent.width
        height: 1
        color: Qt.rgba(1, 1, 1, 0.05)
        anchors.top: parent.top
    }

    // Material Symbols font
    FontLoader {
        id: materialFont
        source: "qrc:/fonts/MaterialSymbolsOutlined.ttf"
    }

    Row {
        anchors.fill: parent
        anchors.bottomMargin: root.safeBottom // padding handled by safeBottom

        Repeater {
            // Material Symbols codepoints:
            // explore: e87a, grid_view: e9b0, forum: e0bf, account_circle: f20b
            model: [
                { icon: "\ue87a", label: "EXPLORE" },
                { icon: "\ue9b0", label: "GALLERY" },
                { icon: "\ue0bf", label: "COMMUNITY" },
                { icon: "\uf20b", label: "PROFILE" }
            ]

            delegate: Item {
                width: root.width / 4
                height: parent.height

                property bool isActive: root.currentIndex === index

                Column {
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.icon
                        font.pixelSize: 26
                        font.family: materialFont.name
                        color: isActive ? "#E13535" : "#64748B"
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.label
                        font.pixelSize: 10
                        font.weight: Font.Medium
                        font.letterSpacing: 0.5
                        color: isActive ? "#E13535" : "#64748B"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.currentIndex = index;
                        root.tabClicked(index);
                    }
                }
            }
        }
    }
}
