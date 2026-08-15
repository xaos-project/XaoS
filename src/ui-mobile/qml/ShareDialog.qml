import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import "."

/*
 * ShareDialog — Upload dialog for sharing the current fractal
 * to the community server. Captures title + author, auto-generates
 * thumbnail, and sends XPF data + metadata.
 */
ThemedPopup {
    id: shareDialog

    accent: Theme.accentGreen

    property bool shareSuccess: false

    onOpened: {
        titleField.text = ""
        authorField.text = ""
        shareSuccess = false
        statusLabel.text = ""
        statusLabel.visible = false
    }

    Connections {
        target: community
        function onUploadComplete(id) {
            shareSuccess = true
            statusLabel.text = "Shared successfully"
            statusLabel.tone = Theme.accentGreen
            statusLabel.visible = true
        }
        function onNetworkError(message) {
            statusLabel.text = message
            statusLabel.tone = Theme.danger
            statusLabel.visible = true
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.s4

        Column {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: "SHARE"
                font.pixelSize: Theme.fontEyebrow
                font.bold: true
                font.letterSpacing: Theme.trackingWide
                color: Theme.textDim
            }
            Text {
                text: "Share Fractal"
                font.pixelSize: Theme.fontXl
                font.bold: true
                color: Theme.textPrimary
            }
            Text {
                width: parent.width
                text: "Publish your creation to the XaoS community."
                font.pixelSize: Theme.fontSm + 1
                color: Theme.textSecondary
                wrapMode: Text.Wrap
            }
        }

        // Current fractal summary.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
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
                    icon: "auto_awesome"
                    size: 32
                    iconColor: Theme.accentAmber
                    bgColor: Theme.alpha(Theme.accentAmber, 0.10)
                    borderColor: Theme.alpha(Theme.accentAmber, 0.18)
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 3

                    Text {
                        width: parent.width
                        text: bridge ? bridge.formulaName : "Mandelbrot"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontMd
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Text {
                        text: "Iter " + (bridge ? bridge.maxIterations : "—") +
                              "   ·   Zoom " + (bridge ? bridge.zoomLevel : "1×")
                        color: Theme.textDim
                        font.pixelSize: Theme.fontSm
                        font.family: "monospace"
                    }
                }
            }
        }

        // Destination.
        Column {
            Layout.fillWidth: true
            spacing: Theme.s1

            Text {
                text: "POSTING TO"
                font.pixelSize: Theme.fontEyebrow
                font.bold: true
                font.letterSpacing: Theme.trackingTight
                color: Theme.textDim
            }

            Basic.ComboBox {
                id: targetGroupCombo
                width: parent.width
                height: 44
                textRole: "text"
                valueRole: "value"
                model: {
                    var list = [{ text: "Public Gallery", value: -1 }]
                    if (community && community.userRooms) {
                        for (var i = 0; i < community.userRooms.length; i++) {
                            list.push({ text: "Room: " + community.userRooms[i].name, value: community.userRooms[i].id })
                        }
                    }
                    return list
                }
                Component.onCompleted: updateSelection()
                onModelChanged: updateSelection()

                function updateSelection() {
                    if (community && community.currentGroupId > 0) {
                        for (var i = 0; i < count; i++) {
                            if (model[i].value === community.currentGroupId) {
                                currentIndex = i
                                break
                            }
                        }
                    } else {
                        currentIndex = 0
                    }
                }

                background: Rectangle {
                    radius: Theme.radiusMd
                    color: Theme.bgSurface
                    border.color: targetGroupCombo.hovered || targetGroupCombo.popup.visible
                                  ? Theme.borderBright : Theme.borderSubtle
                    border.width: 1
                    Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
                }

                contentItem: Text {
                    leftPadding: Theme.s3
                    rightPadding: Theme.s3
                    text: targetGroupCombo.currentText
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontMd
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                indicator: Text {
                    x: targetGroupCombo.width - width - Theme.s3
                    y: (targetGroupCombo.height - height) / 2
                    text: "expand_more"
                    font.family: Theme.iconFont
                    font.pixelSize: Theme.fontXl
                    color: Theme.textSecondary
                }

                popup: Popup {
                    y: targetGroupCombo.height + 4
                    width: targetGroupCombo.width
                    implicitHeight: Math.min(contentItem.implicitHeight + 8, 240)
                    padding: 4

                    background: Rectangle {
                        radius: Theme.radiusMd
                        color: Theme.bgElevated
                        border.color: Theme.borderBright
                        border.width: 1
                    }

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: targetGroupCombo.delegateModel
                        currentIndex: targetGroupCombo.highlightedIndex
                    }
                }

                delegate: Basic.ItemDelegate {
                    width: targetGroupCombo.width - 8
                    height: 38

                    background: Rectangle {
                        radius: Theme.radiusSm
                        color: highlighted ? Theme.alpha(Theme.accentCyan, 0.12) : "transparent"
                    }

                    contentItem: Text {
                        leftPadding: Theme.s2
                        text: modelData.text
                        color: highlighted ? Theme.accentCyan : Theme.textPrimary
                        font.pixelSize: Theme.fontBody
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    highlighted: targetGroupCombo.highlightedIndex === index
                }
            }
        }

        ThemedField {
            id: titleField
            Layout.fillWidth: true
            label: "TITLE *"
            placeholderText: "Give your fractal a name…"
            maximumLength: 100
            accent: Theme.accentGreen
        }

        ThemedField {
            id: authorField
            Layout.fillWidth: true
            visible: !(community && community.isLoggedIn)
            label: "YOUR NAME (OPTIONAL)"
            placeholderText: "Anonymous"
            maximumLength: 50
            accent: Theme.accentGreen
        }

        Text {
            id: statusLabel
            property color tone: Theme.accentGreen

            Layout.fillWidth: true
            visible: false
            font.pixelSize: Theme.fontBody
            color: tone
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }

        PrimaryButton {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
            accent: Theme.accentCyan
            iconGlyph: shareSuccess ? "check_circle" : "cloud_upload"
            text: shareSuccess ? "Done" :
                  (community && community.loading ? "Sharing…" : "Share")
            enabled: !shareSuccess && !(community && community.loading) &&
                     titleField.text.trim().length > 0
            // Once shared, the button is inert but should read as success.
            disabledFill: shareSuccess ? Theme.accentGreen : Theme.bgCard
            disabledText: shareSuccess ? Theme.textOnAccent : Theme.textDim

            onClicked: {
                if (!bridge || !community) return

                var thumbPath = bridge.getTempPath("xaos_share_thumb.png")
                bridge.saveThumbnail(thumbPath)

                var xpfData = bridge.getCurrentXpf()

                community.upload(
                    titleField.text.trim(),
                    authorField.text.trim() || "Anonymous",
                    xpfData,
                    thumbPath,
                    bridge.formulaName,
                    bridge.maxIterations,
                    bridge.zoomLevel,
                    targetGroupCombo.currentValue
                )
            }
        }

        GhostButton {
            Layout.fillWidth: true
            visible: shareSuccess
            text: "Close"
            accent: Theme.textSecondary
            onClicked: shareDialog.close()
        }
    }
}
