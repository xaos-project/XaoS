import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

/*
 * TeacherDashboard — teacher login / signup dialog.
 */
ThemedPopup {
    id: teacherLoginPopup

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property int currentTab: 0
    onCurrentTabChanged: {
        if (community) community.clearError()
    }

    Timer {
        id: errorClearTimer
        interval: 3500
        repeat: false
        onTriggered: {
            if (community) community.clearError()
        }
    }

    Connections {
        target: community
        function onErrorChanged() {
            if (community && community.errorMessage !== "") {
                errorClearTimer.restart()
            }
        }
    }

    onOpened: {
        emailInput.text = ""
        passwordInput.text = ""
        displayNameInput.text = ""
        if (community) {
            community.clearError()
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.s4

        SegmentedControl {
            Layout.fillWidth: true
            options: ["Login", "Sign Up"]
            glyphs: ["login", "person_add"]
            currentIndex: teacherLoginPopup.currentTab
            onActivated: function(index) { teacherLoginPopup.currentTab = index }
        }

        Column {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
            spacing: Theme.s1

            Text {
                text: "TEACHER ACCOUNT"
                font.pixelSize: Theme.fontEyebrow
                font.bold: true
                font.letterSpacing: Theme.trackingWide
                color: Theme.textDim
            }
            Text {
                width: parent.width
                text: teacherLoginPopup.currentTab === 0
                      ? "Welcome Back, Teacher" : "Create a Teacher Account"
                font.pixelSize: Theme.fontXl
                font.bold: true
                color: Theme.textPrimary
                wrapMode: Text.Wrap
            }
        }

        ThemedField {
            id: emailInput
            Layout.fillWidth: true
            label: "EMAIL ADDRESS"
            placeholderText: "you@school.edu"
            inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase
        }

        ThemedField {
            id: displayNameInput
            Layout.fillWidth: true
            visible: teacherLoginPopup.currentTab === 1
            label: "DISPLAY NAME"
            placeholderText: "e.g. Mr. Smith"
        }

        ThemedField {
            id: passwordInput
            Layout.fillWidth: true
            label: "PASSWORD"
            placeholderText: "••••••••"
            echoMode: TextInput.Password
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: errorText.implicitHeight + Theme.s3
            visible: community ? !!community.errorMessage : false
            radius: Theme.radiusSm
            color: Theme.alpha(Theme.danger, 0.10)
            border.color: Theme.alpha(Theme.danger, 0.30)
            border.width: 1

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Theme.s2
                anchors.rightMargin: Theme.s2
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.s2

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "error_outline"
                    font.family: Theme.iconFont
                    font.pixelSize: Theme.fontLg
                    color: Theme.danger
                }
                Text {
                    id: errorText
                    width: parent.width - Theme.fontLg - Theme.s2
                    anchors.verticalCenter: parent.verticalCenter
                    text: community ? community.errorMessage : ""
                    color: Theme.danger
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.Wrap
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s1
            spacing: Theme.s3

            GhostButton {
                Layout.fillWidth: true
                text: "Cancel"
                accent: Theme.textSecondary
                onClicked: teacherLoginPopup.close()
            }

            PrimaryButton {
                Layout.fillWidth: true
                text: teacherLoginPopup.currentTab === 0 ? "Login" : "Sign Up"
                iconGlyph: teacherLoginPopup.currentTab === 0 ? "login" : "person_add"
                enabled: {
                    if (community && community.loading) return false;
                    if (teacherLoginPopup.currentTab === 0)
                        return emailInput.text.length > 0 && passwordInput.text.length > 0;
                    return emailInput.text.length > 0 && passwordInput.text.length > 0 && displayNameInput.text.length > 0;
                }
                onClicked: {
                    if (community) {
                        if (teacherLoginPopup.currentTab === 0) {
                            community.teacherLogin(emailInput.text, passwordInput.text)
                        } else {
                            community.teacherSignup(emailInput.text, passwordInput.text, displayNameInput.text)
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: community
        function onLoginSuccess() {
            if (teacherLoginPopup.visible) {
                teacherLoginPopup.close()
            }
        }
    }
}
