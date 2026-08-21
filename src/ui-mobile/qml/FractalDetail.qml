import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

/*
 * FractalDetail — Detail view shown when tapping a gallery item.
 * Displays full thumbnail, metadata, and a "Download & Explore" button.
 */
ThemedPopup {
    id: detailPopup

    accent: Theme.accentMagenta

    property var fractalData: ({})

    property bool alreadyLiked: false
    property bool likeSending: false
    property int likeCount: 0
    property bool showAlreadyLikedHint: false

    Timer {
        id: alreadyLikedHintTimer
        interval: 2000
        onTriggered: detailPopup.showAlreadyLikedHint = false
    }

    // Lets the gallery keep the card's count in step with the detail view.
    signal likeApplied(int fractalId, int delta)

    function refreshLiked() {
        var id = (community && fractalData) ? fractalData.id : 0
        alreadyLiked = id ? community.hasLiked(id) : false
        likeSending = id ? community.likePending(id) : false
        likeCount = (fractalData && fractalData.likes) ? fractalData.likes : 0
    }

    function applyLikeDelta(delta) {
        var id = (fractalData && fractalData.id) ? fractalData.id : 0
        if (!id)
            return
        likeCount = Math.max(0, likeCount + delta)
        fractalData.likes = likeCount
        likeApplied(id, delta)
    }

    onOpened: {
        showAlreadyLikedHint = false
        refreshLiked()
    }
    onFractalDataChanged: refreshLiked()

    Connections {
        target: community
        function onLikedFractalsChanged() { detailPopup.refreshLiked() }
        function onLikeFailed(fractalId) {
            if (detailPopup.fractalData && detailPopup.fractalData.id === fractalId)
                detailPopup.applyLikeDelta(-1)
        }
        function onXpfDownloaded(fractalId, xpfData) {
            if (bridge) {
                bridge.loadFromXpf(xpfData)
            }
            detailPopup.close()
            if (detailPopup.parent && detailPopup.parent.close)
                detailPopup.parent.close()
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.s3

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.s2

            Column {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: "FRACTAL"
                    font.pixelSize: Theme.fontEyebrow
                    font.bold: true
                    font.letterSpacing: Theme.trackingWide
                    color: Theme.textDim
                }
                Text {
                    width: parent.width
                    text: detailPopup.fractalData.title || "Untitled"
                    font.pixelSize: Theme.fontXl
                    font.bold: true
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                }
                Text {
                    text: "by " + (detailPopup.fractalData.author || "Anonymous")
                    font.pixelSize: Theme.fontSm + 1
                    color: Theme.textSecondary
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignTop
                width: 34
                height: 34
                radius: Theme.radiusMd
                color: closeArea.pressed ? Theme.alpha(Theme.textPrimary, 0.14)
                                         : closeArea.containsMouse ? Theme.alpha(Theme.textPrimary, 0.08)
                                                                   : "transparent"
                border.color: closeArea.containsMouse ? Theme.borderBright : Theme.borderSubtle
                border.width: 1

                Behavior on color { ColorAnimation { duration: Theme.durFast } }

                Text {
                    anchors.centerIn: parent
                    text: "close"
                    font.family: Theme.iconFont
                    font.pixelSize: Theme.fontXl
                    color: Theme.textSecondary
                }

                MouseArea {
                    id: closeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: detailPopup.close()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: width * 0.7
            radius: Theme.radiusLg
            color: Theme.bgSurface
            border.color: Theme.borderSubtle
            border.width: 1
            clip: true

            Image {
                anchors.fill: parent
                anchors.margins: 1
                source: detailPopup.fractalData.thumbnailUrl || ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }

            Column {
                anchors.centerIn: parent
                spacing: Theme.s2
                visible: !detailPopup.fractalData.thumbnailUrl

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "image_not_supported"
                    font.family: Theme.iconFont
                    font.pixelSize: 34
                    color: Theme.textDim
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "No preview"
                    font.pixelSize: Theme.fontBody
                    color: Theme.textDim
                }
            }
        }

        // Metadata.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: metaGrid.implicitHeight + Theme.s4
            radius: Theme.radiusMd
            color: Theme.bgCard
            border.color: Theme.borderSubtle
            border.width: 1

            GridLayout {
                id: metaGrid
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: Theme.s3
                anchors.rightMargin: Theme.s3
                columns: 2
                columnSpacing: Theme.s4
                rowSpacing: Theme.s1 + 2

                Text {
                    text: "Formula"
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSm + 1
                }
                Text {
                    Layout.fillWidth: true
                    text: detailPopup.fractalData.formula || "—"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSm + 1
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }

                Text {
                    text: "Iterations"
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSm + 1
                }
                Text {
                    Layout.fillWidth: true
                    text: detailPopup.fractalData.iterations
                          ? detailPopup.fractalData.iterations.toString() : "—"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSm + 1
                    font.family: "monospace"
                    horizontalAlignment: Text.AlignRight
                }

                Text {
                    text: "Zoom"
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSm + 1
                }
                Text {
                    Layout.fillWidth: true
                    text: detailPopup.fractalData.zoomLevel || "—"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSm + 1
                    font.family: "monospace"
                    horizontalAlignment: Text.AlignRight
                }

                Text {
                    text: "Downloads"
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSm + 1
                }
                Text {
                    Layout.fillWidth: true
                    text: (detailPopup.fractalData.downloads || 0).toString()
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSm + 1
                    font.family: "monospace"
                    horizontalAlignment: Text.AlignRight
                }

                Text {
                    text: "Likes"
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSm + 1
                }
                Text {
                    id: likesLabel
                    Layout.fillWidth: true
                    text: detailPopup.likeCount.toString()
                    color: Theme.accentMagenta
                    font.pixelSize: Theme.fontSm + 1
                    font.family: "monospace"
                    font.bold: true
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
            spacing: Theme.s3

            PrimaryButton {
                Layout.fillWidth: true
                iconGlyph: "download"
                text: community && community.loading ? "Downloading…" : "Download & Explore"
                enabled: !(community && community.loading)

                onClicked: {
                    if (community && detailPopup.fractalData.id) {
                        community.downloadXpf(detailPopup.fractalData.id)
                    }
                }
            }

            GhostButton {
                Layout.preferredWidth: 64
                iconGlyph: detailPopup.alreadyLiked ? "favorite" : "favorite_border"
                text: ""
                accent: Theme.accentMagenta
                enabled: !(community && community.loading)
                opacity: detailPopup.likeSending ? 0.5
                       : detailPopup.alreadyLiked ? 0.75 : 1.0

                ToolTip.visible: detailPopup.showAlreadyLikedHint
                ToolTip.text: "Already liked"

                onClicked: {
                    if (!community || !detailPopup.fractalData.id
                            || detailPopup.likeSending)
                        return

                    if (detailPopup.alreadyLiked) {
                        detailPopup.showAlreadyLikedHint = true
                        alreadyLikedHintTimer.restart()
                        return
                    }

                    community.likeFractal(detailPopup.fractalData.id)
                    detailPopup.applyLikeDelta(1)
                }
            }
        }
    }
}
