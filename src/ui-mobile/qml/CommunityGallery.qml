import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import "."

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

    readonly property bool isWide: width >= Theme.wideBreakpoint

    // The body is a centred column; the header title aligns to its left edge.
    readonly property real contentPad: isWide ? Theme.s6 : Theme.s4
    readonly property real contentWidth:
        Math.min(width - contentPad * 2, Theme.maxContentWidth)

    Rectangle {
        anchors.fill: parent
        color: Theme.bgDark
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
        spacing: 0

        ScreenHeader {
            subtitle: "COMMUNITY"
            title: "XaoS Community"
            countAccent: Theme.accentMagenta
            wide: galleryPopup.isWide
            contentInset: Math.max(galleryPopup.contentPad,
                                   (galleryPopup.width - galleryPopup.contentWidth) / 2)
            showCount: galleryPopup.galleryItems.length > 0
            countText: galleryPopup.galleryItems.length +
                       (galleryPopup.galleryItems.length === 1 ? " fractal" : " fractals")

            GhostButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: community ? community.isLoggedIn : false
                compact: true
                text: "Logout"
                iconGlyph: "logout"
                accent: Theme.textSecondary
                onClicked: {
                    if (community) {
                        community.logout()
                    }
                }
            }
        }

        // Wide windows would otherwise stretch the gallery edge to edge.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.topMargin: Theme.s3
                width: galleryPopup.contentWidth
                spacing: Theme.s3

                SegmentedControl {
                    Layout.fillWidth: true
                    Layout.maximumWidth: galleryPopup.isWide ? 420 : Number.POSITIVE_INFINITY
                    Layout.alignment: Qt.AlignHCenter
                    options: ["Public", "Private Rooms"]
                    glyphs: ["public", "lock"]
                    accent: Theme.accentMagenta
                    currentIndex: galleryPopup.currentTab
                    onActivated: function(index) {
                        galleryPopup.currentTab = index
                        if (index === 0) {
                            switchToPublic()
                        } else {
                            switchToPrivate()
                        }
                    }
                }

                SegmentedControl {
                    Layout.fillWidth: true
                    Layout.maximumWidth: galleryPopup.isWide ? 320 : Number.POSITIVE_INFINITY
                    Layout.alignment: Qt.AlignHCenter
                    visible: galleryPopup.currentTab === 0
                    options: ["Recent", "Popular"]
                    glyphs: ["schedule", "trending_up"]
                    currentIndex: galleryPopup.sortMode === "recent" ? 0 : 1
                    onActivated: function(index) {
                        galleryPopup.sortMode = index === 0 ? "recent" : "popular"
                        galleryPopup.currentPage = 1
                        refresh()
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    visible: galleryPopup.currentTab === 1 && community && community.isLoggedIn
                    orientation: ListView.Horizontal
                    spacing: Theme.s2
                    clip: true
                    model: community ? community.userRooms : []

                    delegate: GhostButton {
                        height: 38
                        compact: true
                        text: modelData.name
                        iconGlyph: "meeting_room"
                        accent: (community && community.currentGroupId === modelData.id)
                                ? Theme.accentCyan : Theme.textSecondary
                        onClicked: {
                            community.selectRoom(modelData.id, modelData.name)
                            galleryPopup.currentPage = 1
                            refresh()
                        }
                    }

                    footer: GhostButton {
                        height: 38
                        compact: true
                        accent: Theme.accentMagenta
                        iconGlyph: "add"
                        text: community && community.currentUserRole === "teacher"
                              ? "Create Room" : "Join Room"
                        onClicked: {
                            if (community && community.currentUserRole === "teacher") {
                                createRoomPopup.open()
                            } else {
                                joinGroupPopup.open()
                            }
                        }
                    }
                }

                // Error banner — tap to dismiss and retry.
                Rectangle {
                    id: errorLabel
                    property alias text: errorText.text

                    Layout.fillWidth: true
                    Layout.preferredHeight: errorText.implicitHeight + Theme.s4
                    visible: false
                    radius: Theme.radiusMd
                    color: Theme.alpha(Theme.danger, 0.10)
                    border.color: Theme.alpha(Theme.danger, 0.30)
                    border.width: 1

                    Row {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: Theme.s3
                        anchors.rightMargin: Theme.s3
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.s2

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "error_outline"
                            font.family: Theme.iconFont
                            font.pixelSize: Theme.fontXl
                            color: Theme.danger
                        }

                        Text {
                            id: errorText
                            width: parent.width - Theme.fontXl - Theme.s2
                            anchors.verticalCenter: parent.verticalCenter
                            font.pixelSize: Theme.fontBody
                            color: Theme.danger
                            wrapMode: Text.Wrap
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { errorLabel.visible = false; refresh() }
                    }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: (galleryPopup.currentTab === 1 && community && !community.isLoggedIn) ? 1 : 0

                    Item {
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: Theme.s2

                            BusyIndicator {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredHeight: 40
                                running: community ? community.loading : false
                                visible: running
                            }

                            // Room identity strip for the selected private room.
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                visible: galleryPopup.currentTab === 1 && community && community.currentGroupId > 0
                                radius: Theme.radiusMd
                                color: Theme.bgCard
                                border.color: Theme.borderSubtle
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.s3
                                    anchors.rightMargin: Theme.s3
                                    spacing: Theme.s3

                                    IconBadge {
                                        icon: "meeting_room"
                                        size: 30
                                        iconColor: Theme.accentMagenta
                                        bgColor: Theme.alpha(Theme.accentMagenta, 0.10)
                                        borderColor: Theme.alpha(Theme.accentMagenta, 0.20)
                                    }

                                    Column {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text {
                                            text: "ROOM"
                                            font.pixelSize: Theme.fontEyebrow
                                            font.bold: true
                                            font.letterSpacing: Theme.trackingTight
                                            color: Theme.textDim
                                        }
                                        Text {
                                            width: parent.width
                                            text: community ? community.currentGroupName : ""
                                            font.pixelSize: Theme.fontMd
                                            font.weight: Font.DemiBold
                                            color: Theme.textPrimary
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Column {
                                        spacing: 2
                                        visible: inviteCodeText.text.length > 0
                                        Text {
                                            anchors.right: parent.right
                                            text: "INVITE CODE"
                                            font.pixelSize: Theme.fontEyebrow
                                            font.bold: true
                                            font.letterSpacing: Theme.trackingTight
                                            color: Theme.textDim
                                        }
                                        Text {
                                            id: inviteCodeText
                                            anchors.right: parent.right
                                            text: {
                                                if (!community || !community.userRooms) return "";
                                                for (var i = 0; i < community.userRooms.length; i++) {
                                                    if (community.userRooms[i].id === community.currentGroupId) {
                                                        return community.userRooms[i].inviteCode;
                                                    }
                                                }
                                                return "";
                                            }
                                            font.pixelSize: Theme.fontMd
                                            font.family: "monospace"
                                            font.bold: true
                                            font.letterSpacing: Theme.trackingTight
                                            color: Theme.accentMagenta
                                        }
                                    }
                                }
                            }

                            EmptyState {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                visible: galleryPopup.galleryItems.length === 0 && !(community && community.loading)
                                accent: Theme.accentMagenta
                                glyph: galleryPopup.currentTab === 0 ? "auto_awesome" : "folder_open"
                                title: galleryPopup.currentTab === 0
                                       ? "No public fractals yet"
                                       : "Nothing shared in this room"
                                subtitle: galleryPopup.currentTab === 0
                                          ? "Be the first to share one — explore a fractal, then tap Share."
                                          : "Fractals shared to this room will appear here."
                            }

                            GridView {
                                id: gridView
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                visible: galleryPopup.galleryItems.length > 0
                                clip: true

                                // Columns follow the available width instead of
                                // being pinned at two, which made desktop cards huge.
                                readonly property int columns:
                                    Math.max(2, Math.min(5, Math.floor(width / 260)))

                                cellWidth: width / columns
                                cellHeight: cellWidth * 0.78 + 60
                                model: galleryPopup.galleryItems

                                ScrollBar.vertical: Basic.ScrollBar {
                                    policy: ScrollBar.AsNeeded
                                    contentItem: Rectangle {
                                        implicitWidth: 3
                                        radius: 1.5
                                        color: Theme.alpha(Theme.accentCyan, 0.25)
                                    }
                                }

                                delegate: Item {
                                    width: gridView.cellWidth
                                    height: gridView.cellHeight

                                    Rectangle {
                                        anchors.fill: parent
                                        anchors.margins: Theme.s1 + 1
                                        radius: Theme.radiusLg
                                        color: Theme.bgCard
                                        border.color: delegateArea.pressed
                                                      ? Theme.accentMagenta
                                                      : delegateArea.containsMouse
                                                        ? Theme.alpha(Theme.accentMagenta, 0.45)
                                                        : Theme.borderSubtle
                                        border.width: 1
                                        clip: true

                                        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 1
                                            spacing: 0

                                            Rectangle {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                color: Theme.bgSurface
                                                clip: true

                                                Image {
                                                    anchors.fill: parent
                                                    source: modelData.thumbnailUrl || ""
                                                    fillMode: Image.PreserveAspectCrop
                                                    asynchronous: true
                                                    scale: delegateArea.containsMouse ? 1.04 : 1.0
                                                    Behavior on scale {
                                                        NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic }
                                                    }
                                                }

                                                Text {
                                                    anchors.centerIn: parent
                                                    visible: !modelData.thumbnailUrl
                                                    text: "image"
                                                    font.family: Theme.iconFont
                                                    font.pixelSize: 32
                                                    color: Theme.textDim
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                Layout.margins: Theme.s2
                                                spacing: Theme.s1

                                                Text {
                                                    Layout.fillWidth: true
                                                    text: modelData.title || "Untitled"
                                                    color: Theme.textPrimary
                                                    font.pixelSize: Theme.fontBody
                                                    font.weight: Font.DemiBold
                                                    elide: Text.ElideRight
                                                }

                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    spacing: Theme.s2

                                                    Text {
                                                        Layout.fillWidth: true
                                                        text: modelData.author || "Anonymous"
                                                        color: Theme.textSecondary
                                                        font.pixelSize: Theme.fontSm
                                                        elide: Text.ElideRight
                                                    }

                                                    StatChip {
                                                        glyph: "favorite"
                                                        value: (modelData.likes || 0).toString()
                                                        accent: Theme.accentMagenta
                                                    }

                                                    StatChip {
                                                        glyph: "download"
                                                        value: (modelData.downloads || 0).toString()
                                                        accent: Theme.textDim
                                                    }
                                                }
                                            }
                                        }

                                        MouseArea {
                                            id: delegateArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
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
                                Layout.bottomMargin: Theme.s2
                                spacing: Theme.s3
                                visible: galleryPopup.totalPages > 1

                                Item { Layout.fillWidth: true }

                                GhostButton {
                                    compact: true
                                    text: "Prev"
                                    iconGlyph: "chevron_left"
                                    enabled: galleryPopup.currentPage > 1
                                    onClicked: { galleryPopup.currentPage--; refresh() }
                                }

                                Text {
                                    text: galleryPopup.currentPage + " / " + galleryPopup.totalPages
                                    font.pixelSize: Theme.fontBody
                                    font.family: "monospace"
                                    color: Theme.textSecondary
                                }

                                GhostButton {
                                    compact: true
                                    text: "Next"
                                    iconGlyph: "chevron_right"
                                    enabled: galleryPopup.currentPage < galleryPopup.totalPages
                                    onClicked: { galleryPopup.currentPage++; refresh() }
                                }

                                Item { Layout.fillWidth: true }
                            }

                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.bottomMargin: Theme.s3
                                spacing: Theme.s3
                                visible: galleryPopup.currentTab === 1 && community && community.currentGroupId > 0

                                GhostButton {
                                    compact: true
                                    text: "View Members"
                                    iconGlyph: "group"
                                    accent: Theme.accentPurple
                                    onClicked: {
                                        if (community) {
                                            community.fetchRoomMembers(community.currentGroupId)
                                            roomMembersPopup.open()
                                        }
                                    }
                                }

                                GhostButton {
                                    compact: true
                                    text: "Leave Room"
                                    iconGlyph: "logout"
                                    accent: Theme.textSecondary
                                    onClicked: {
                                        if (community) {
                                            community.leaveRoom(community.currentGroupId)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        EmptyState {
                            id: signedOutState
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: Math.max(Theme.s6, parent.height * 0.15)
                            width: Math.min(parent.width, 400)
                            height: implicitHeight
                            accent: Theme.accentMagenta
                            glyph: "lock"
                            title: "Join a Private Room"
                            subtitle: "Enter a code provided by your teacher to access a private class space."
                        }

                        ColumnLayout {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: signedOutState.bottom
                            anchors.topMargin: Theme.s6
                            width: Math.min(parent.width * 0.86, 340)
                            spacing: Theme.s4

                            PrimaryButton {
                                Layout.fillWidth: true
                                text: "Join a Room"
                                iconGlyph: "login"
                                accent: Theme.accentMagenta
                                onClicked: joinGroupPopup.open()
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: Theme.borderSubtle
                            }

                            GhostButton {
                                Layout.fillWidth: true
                                text: "Log in as Teacher"
                                iconGlyph: "school"
                                onClicked: teacherLoginPopup.open()
                            }
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
}
