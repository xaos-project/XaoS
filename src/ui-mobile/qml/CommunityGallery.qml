import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/*
 * CommunityHub — Full-screen overlay for the Community Feature.
 * Contains two main spaces: Public Community and Private Rooms.
 */
Item {
    id: galleryPopup
    anchors.fill: parent

    property int currentPage: 1
    property int totalPages: 1
    property string sortMode: "recent"
    property var galleryItems: []
    property int currentTab: 0

    Rectangle {
        anchors.fill: parent
        color: "#1a1a2e"
    }

    onVisibleChanged: {
        if (visible) {
            currentTab = 0
            switchToPublic()
        }
    }

    function switchToPublic() {
        if (community) {
            community.selectRoom(-1, "")
            currentPage = 1
            galleryItems = []
            refresh()
        }
    }

    function switchToPrivate() {
        if (community) {
            if (community.isLoggedIn && community.userRooms && community.userRooms.length > 0) {
                if (community.currentGroupId === -1) {
                    var firstRoom = community.userRooms[0]
                    community.selectRoom(firstRoom.id, firstRoom.name)
                }
            } else {
                community.selectRoom(-1, "")
            }
            currentPage = 1
            galleryItems = []
            refresh()
        }
    }

    function refresh() {
        errorLabel.visible = false
        if (community) {
            if (currentTab === 0 || (currentTab === 1 && community.currentGroupId > 0)) {
                community.fetchGallery(currentPage, sortMode)
            }
        }
    }

    Connections {
        target: community
        function onGalleryLoaded(items, pages, page) {
            errorLabel.visible = false
            galleryPopup.totalPages = pages
            galleryPopup.currentPage = page
            galleryPopup.galleryItems = items
        }
        function onNetworkError(message) {
            errorLabel.text = message
            errorLabel.visible = true
        }
        function onAuthChanged() {
            if (currentTab === 1) {
                switchToPrivate()
            }
        }
        function onUserRoomsChanged() {
            if (currentTab === 1) {
                switchToPrivate()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "XaoS Community"
                font.pixelSize: 22
                font.bold: true
                color: "#e94560"
                Layout.fillWidth: true
            }

            Button {
                text: "Logout"
                visible: community ? community.isLoggedIn : false
                onClicked: {
                    if (community) {
                        community.logout()
                    }
                }
                background: Rectangle {
                    color: parent.pressed ? "#333" : "transparent"
                    border.color: "#555"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; color: "#aaa"
                    font.pixelSize: 14; font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
            }

        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Public Community"
                Layout.fillWidth: true
                highlighted: currentTab === 0
                onClicked: {
                    currentTab = 0
                    switchToPublic()
                }
                background: Rectangle {
                    color: currentTab === 0 ? "#e94560" : (parent.pressed ? "#0f3460" : "#16213e")
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.pixelSize: 16; font.bold: currentTab === 0
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                text: "Private Rooms"
                Layout.fillWidth: true
                highlighted: currentTab === 1
                onClicked: {
                    currentTab = 1
                    switchToPrivate()
                }
                background: Rectangle {
                    color: currentTab === 1 ? "#e94560" : (parent.pressed ? "#0f3460" : "#16213e")
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.pixelSize: 16; font.bold: currentTab === 1
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: currentTab === 0

            Button {
                text: "Recent"
                Layout.fillWidth: true
                highlighted: sortMode === "recent"
                onClicked: {
                    sortMode = "recent"
                    currentPage = 1
                    refresh()
                }
                background: Rectangle {
                    color: sortMode === "recent" ? "#0f3460" : (parent.pressed ? "#333" : "#111")
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.pixelSize: 14; font.bold: sortMode === "recent"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                text: "Popular"
                Layout.fillWidth: true
                highlighted: sortMode === "popular"
                onClicked: {
                    sortMode = "popular"
                    currentPage = 1
                    refresh()
                }
                background: Rectangle {
                    color: sortMode === "popular" ? "#0f3460" : (parent.pressed ? "#333" : "#111")
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.pixelSize: 14; font.bold: sortMode === "popular"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            visible: currentTab === 1 && community && community.isLoggedIn
            orientation: ListView.Horizontal
            spacing: 8
            clip: true
            model: community ? community.userRooms : []

            delegate: Button {
                text: modelData.name
                highlighted: community && community.currentGroupId === modelData.id
                onClicked: {
                    community.selectRoom(modelData.id, modelData.name)
                    currentPage = 1
                    refresh()
                }
                background: Rectangle {
                    color: parent.highlighted ? "#0f3460" : (parent.pressed ? "#333" : "#111")
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.pixelSize: 14; font.bold: parent.highlighted
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            footer: Button {
                text: community && community.currentUserRole === "teacher" ? "+ Create Room" : "+ Join Room"
                onClicked: {
                    if (community && community.currentUserRole === "teacher") {
                        createRoomPopup.open()
                    } else {
                        joinGroupPopup.open()
                    }
                }
                background: Rectangle {
                    color: parent.pressed ? "#333" : "#111"
                    radius: 8
                    border.color: "#e94560"
                    border.width: 1
                }
                contentItem: Text {
                    text: parent.text; color: "#e94560"
                    font.pixelSize: 14; font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: (currentTab === 1 && community && !community.isLoggedIn) ? 1 : 0

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    BusyIndicator {
                        Layout.alignment: Qt.AlignHCenter
                        running: community ? community.loading : false
                        visible: running
                    }

                    Label {
                        id: errorLabel
                        visible: false
                        color: "#ff6b6b"
                        font.pixelSize: 13
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        MouseArea {
                            anchors.fill: parent
                            onClicked: { errorLabel.visible = false; refresh() }
                        }
                    }

                    Label {
                        visible: galleryItems.length === 0 && !(community && community.loading)
                        text: currentTab === 0 ? "No public fractals shared yet.\nBe the first to share!" : "No fractals in this room yet."
                        color: "#888"
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                    }
                    
                    RowLayout {
                        visible: currentTab === 1 && community && community.currentGroupId > 0
                        Layout.fillWidth: true
                        Layout.margins: 4
                        
                        Label {
                            text: community ? "Room: " + community.currentGroupName : ""
                            color: "#fff"
                            font.pixelSize: 16
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        Label {
                            text: {
                                if (!community || !community.userRooms) return "";
                                for (var i = 0; i < community.userRooms.length; i++) {
                                    if (community.userRooms[i].id === community.currentGroupId) {
                                        return "Invite Code: " + community.userRooms[i].inviteCode;
                                    }
                                }
                                return "";
                            }
                            color: "#e94560"
                            font.pixelSize: 14
                            font.bold: true
                        }
                    }

                    GridView {
                        id: gridView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        cellWidth: (width - 8) / 2
                        cellHeight: cellWidth + 48
                        model: galleryItems

                        delegate: Item {
                            width: gridView.cellWidth
                            height: gridView.cellHeight

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 4
                                radius: 12
                                color: "#16213e"
                                border.color: delegateArea.pressed ? "#e94560" : "#0f3460"
                                border.width: 1
                                clip: true

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 0

                                    Image {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        Layout.margins: 4
                                        source: modelData.thumbnailUrl || ""
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true

                                        Rectangle {
                                            anchors.fill: parent
                                            color: "#0f3460"
                                            visible: parent.status !== Image.Ready
                                            Label {
                                                anchors.centerIn: parent
                                                text: "\ue3b6"
                                                font.family: materialFont.name
                                                font.pixelSize: 32
                                                color: "#444"
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.margins: 8
                                        Layout.topMargin: 4
                                        spacing: 2

                                        Label {
                                            text: modelData.title || "Untitled"
                                            color: "#fff"
                                            font.pixelSize: 13
                                            font.bold: true
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            Label {
                                                text: modelData.author || "Anonymous"
                                                color: "#888"
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                                Layout.fillWidth: true
                                            }
                                            Label {
                                                text: "❤️ " + (modelData.likes || 0)
                                                color: "#e94560"
                                                font.pixelSize: 11
                                            }
                                            Label {
                                                text: "\u2B07 " + (modelData.downloads || 0)
                                                color: "#666"
                                                font.pixelSize: 11
                                            }
                                        }
                                    }
                                }

                                MouseArea {
                                    id: delegateArea
                                    anchors.fill: parent
                                    onClicked: {
                                        detailPopup.fractalData = modelData
                                        detailPopup.open()
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 16
                        visible: totalPages > 1

                        Button {
                            text: "\u25C0 Prev"
                            enabled: currentPage > 1
                            onClicked: { currentPage--; refresh() }
                            background: Rectangle {
                                color: parent.enabled ? (parent.pressed ? "#0f3460" : "#16213e") : "#111"
                                radius: 8
                            }
                            contentItem: Text {
                                text: parent.text
                                color: parent.enabled ? "#fff" : "#555"
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        Label {
                            text: currentPage + " / " + totalPages
                            color: "#aaa"
                            font.pixelSize: 14
                        }

                        Button {
                            text: "Next \u25B6"
                            enabled: currentPage < totalPages
                            onClicked: { currentPage++; refresh() }
                            background: Rectangle {
                                color: parent.enabled ? (parent.pressed ? "#0f3460" : "#16213e") : "#111"
                                radius: 8
                            }
                            contentItem: Text {
                                text: parent.text
                                color: parent.enabled ? "#fff" : "#555"
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 16
                        visible: currentTab === 1 && community && community.currentGroupId > 0

                        Button {
                            text: "View Members"
                            onClicked: {
                                if (community) {
                                    community.fetchRoomMembers(community.currentGroupId)
                                    roomMembersPopup.open()
                                }
                            }
                            background: Rectangle { color: "transparent"; border.color: "#88aaff"; radius: 4 }
                            contentItem: Text { text: parent.text; color: "#88aaff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
                        }

                        Button {
                            text: "Leave Room"
                            onClicked: {
                                if (community) {
                                    community.leaveRoom(community.currentGroupId)
                                }
                            }
                            background: Rectangle { color: "transparent"; border.color: "#888"; radius: 4 }
                            contentItem: Text { text: parent.text; color: "#888"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 24
                    width: Math.min(400, parent.width * 0.8)

                    Label {
                        text: "Join a Private Room"
                        color: "#fff"
                        font.pixelSize: 20
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Label {
                        text: "Enter a code provided by your teacher to access a private class space."
                        color: "#aaa"
                        font.pixelSize: 14
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Join a Room"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        onClicked: joinGroupPopup.open()
                        background: Rectangle { color: "#e94560"; radius: 8 }
                        contentItem: Text {
                            text: parent.text; color: "#fff"
                            font.pixelSize: 16; font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#333"
                        Layout.margins: 16
                    }

                    Button {
                        text: "Log in as Teacher"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        onClicked: teacherLoginPopup.open()
                        background: Rectangle { color: "#16213e"; border.color: "#0f3460"; radius: 8 }
                        contentItem: Text {
                            text: parent.text; color: "#88aaff"
                            font.pixelSize: 16; font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }
    }

    FractalDetail {
        id: detailPopup
        parent: Overlay.overlay
    }
    JoinGroup {
        id: joinGroupPopup
        parent: Overlay.overlay
    }
    TeacherDashboard {
        id: teacherLoginPopup
        parent: Overlay.overlay
    }
    CreateRoom {
        id: createRoomPopup
        parent: Overlay.overlay
    }
    RoomMembersPopup {
        id: roomMembersPopup
        parent: Overlay.overlay
    }

    Connections {
        target: community
        function onRoomMembersLoaded(members) {
            roomMembersPopup.membersModel = members
        }
    }

    FontLoader {
        id: materialFont
        source: "qrc:/fonts/MaterialIcons-Regular.ttf"
    }
}
