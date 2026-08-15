import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

/*
 * StartScreen — welcome screen offering the three ways into the Community
 * feature. Currently unreferenced; kept in the shared design language so it
 * is ready if the entry flow is ever wired up.
 */
ThemedPopup {
    id: startScreen

    maxWidth: 460
    closePolicy: Popup.NoAutoClose

    contentItem: ColumnLayout {
        spacing: Theme.s4

        Column {
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.s2
            spacing: Theme.s2

            IconBadge {
                anchors.horizontalCenter: parent.horizontalCenter
                icon: "groups"
                size: 52
                iconColor: Theme.accentCyan
            }

            Text {
                width: parent.width
                text: "XAOS"
                font.pixelSize: Theme.fontEyebrow
                font.bold: true
                font.letterSpacing: 5
                color: Theme.textDim
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                width: parent.width
                text: "Welcome to the Community"
                font.pixelSize: Theme.fontDisplay
                font.bold: true
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }

        PrimaryButton {
            Layout.fillWidth: true
            text: "Browse Public Gallery"
            iconGlyph: "public"
            onClicked: {
                startScreen.close()
            }
        }

        PrimaryButton {
            Layout.fillWidth: true
            text: "Join a Group (Students)"
            iconGlyph: "group_add"
            accent: Theme.accentMagenta
            onClicked: {
                startScreen.close()
                joinGroupPopup.open()
            }
        }

        GhostButton {
            Layout.fillWidth: true
            text: "Teacher Sign In"
            iconGlyph: "school"
            accent: Theme.accentMagenta
            onClicked: {
                startScreen.close()
                teacherLoginPopup.open()
            }
        }
    }
}
