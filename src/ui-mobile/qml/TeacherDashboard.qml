import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: teacherLoginPopup
    width: Math.min(600, parent.width * 0.9)
    height: Math.min(500, parent.height * 0.9)
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    modal: true
    focus: true
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

    background: Rectangle {
        color: "#1a1a2e"
        radius: 16
        border.color: "#0f3460"
        border.width: 2
    }

    onOpened: {
        emailInput.text = ""
        passwordInput.text = ""
        displayNameInput.text = ""
        if (community) {
            community.clearError()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            Button {
                text: "Login"
                Layout.fillWidth: true
                highlighted: currentTab === 0
                onClicked: currentTab = 0
                background: Rectangle {
                    color: currentTab === 0 ? "#e94560" : (parent.pressed ? "#333" : "#111")
                    radius: 8
                    Rectangle {
                        width: parent.width; height: 8; anchors.bottom: parent.bottom; color: parent.color; visible: currentTab === 0
                    }
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.pixelSize: 16; font.bold: currentTab === 0
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                text: "Sign Up"
                Layout.fillWidth: true
                highlighted: currentTab === 1
                onClicked: currentTab = 1
                background: Rectangle {
                    color: currentTab === 1 ? "#e94560" : (parent.pressed ? "#333" : "#111")
                    radius: 8
                    Rectangle {
                        width: parent.width; height: 8; anchors.bottom: parent.bottom; color: parent.color; visible: currentTab === 1
                    }
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.pixelSize: 16; font.bold: currentTab === 1
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Label {
            text: currentTab === 0 ? "Welcome Back, Teacher" : "Create a Teacher Account"
            color: "#fff"
            font.pixelSize: 20
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
        }

        TextField {
            id: emailInput
            placeholderText: "Email Address"
            Layout.fillWidth: true
            font.pixelSize: 18
            color: "#000"
            placeholderTextColor: "#666"
            background: Rectangle {
                color: "#fff"
                radius: 8
                border.color: "#ccc"
                border.width: 1
            }
        }

        TextField {
            id: displayNameInput
            placeholderText: "Display Name (e.g. Mr. Smith)"
            visible: currentTab === 1
            Layout.fillWidth: true
            font.pixelSize: 18
            color: "#000"
            placeholderTextColor: "#666"
            background: Rectangle {
                color: "#fff"
                radius: 8
                border.color: "#ccc"
                border.width: 1
            }
        }

        TextField {
            id: passwordInput
            placeholderText: "Password"
            echoMode: TextInput.Password
            Layout.fillWidth: true
            font.pixelSize: 18
            color: "#000"
            placeholderTextColor: "#666"
            background: Rectangle {
                color: "#fff"
                radius: 8
                border.color: "#ccc"
                border.width: 1
            }
        }

        Label {
            visible: community ? !!community.errorMessage : false
            text: community ? community.errorMessage : ""
            color: "#ff6b6b"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Button {
                text: "Cancel"
                Layout.fillWidth: true
                onClicked: teacherLoginPopup.close()
                background: Rectangle { color: "#333"; radius: 8 }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                text: currentTab === 0 ? "Login" : "Sign Up"
                Layout.fillWidth: true
                enabled: {
                    if (community && community.loading) return false;
                    if (currentTab === 0) return emailInput.text.length > 0 && passwordInput.text.length > 0;
                    return emailInput.text.length > 0 && passwordInput.text.length > 0 && displayNameInput.text.length > 0;
                }
                onClicked: {
                    if (community) {
                        if (currentTab === 0) {
                            community.teacherLogin(emailInput.text, passwordInput.text)
                        } else {
                            community.teacherSignup(emailInput.text, passwordInput.text, displayNameInput.text)
                        }
                    }
                }
                background: Rectangle {
                    color: parent.enabled ? "#e94560" : "#555"
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text; color: "#fff"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
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
