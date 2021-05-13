import QtQuick 2.4
import QtQuick.Window 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    title: "Test for Server-Side Add-on"
    width: Screen.width
    height: Screen.height
    visible: true
    color: "orange"
    locationHint: WebOSWindow.LocationHintCenter
    windowType: "_WEBOS_WINDOW_TYPE_SYSTEM_UI"

    property int buttonWidth: 800
    property int buttonHeight: 100

    Column {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 50
        anchors.rightMargin: 50
        spacing: 10

        Rectangle {
            width: root.buttonWidth
            height: root.buttonHeight
            radius: 10
            color: "steelblue"

            property string url: "addon_valid.qml"

            Text {
                anchors.centerIn: parent
                font.pixelSize: 32
                text: "Set addon to " + parent.url
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.addon = Qt.resolvedUrl(parent.url)
                }
            }
        }

        Rectangle {
            width: root.buttonWidth
            height: root.buttonHeight
            radius: 10
            color: "steelblue"

            property string url: "addon_valid.qml"

            Text {
                anchors.centerIn: parent
                font.pixelSize: 32
                text: "Set addon to " + parent.url + " from /tmp"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.addon = "file:///tmp/" + parent.url
                }
            }
        }

        Rectangle {
            width: root.buttonWidth
            height: root.buttonHeight
            radius: 10
            color: "steelblue"

            property string url: "addon_error.qml"

            Text {
                anchors.centerIn: parent
                font.pixelSize: 32
                text: "Set addon to " + parent.url
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.addon = Qt.resolvedUrl(parent.url)
                }
            }
        }

        Rectangle {
            width: root.buttonWidth
            height: root.buttonHeight
            radius: 10
            color: "steelblue"

            Text {
                anchors.centerIn: parent
                font.pixelSize: 32
                text: "reset addon"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.resetAddon();
                }
            }
        }

    }

    Rectangle {
        width: 1200
        height: 150
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.bottomMargin: 50
        anchors.rightMargin: 50
        radius: 10
        color: "coral"

        Text {
            id: addonStatus
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 50
            anchors.left: parent.left
            font.pixelSize: 30
        }
    }


    Connections {
        target: root
        function onAddonStatusChanged(status) {
            console.warn(status);
            var text = "addon: " + root.addon + "\nstatus: ";
            switch (status) {
            case WebOSWindow.AddonStatusNull:
                text += " AddonStatusNull";
                break;
            case WebOSWindow.AddonStatusLoaded:
                text += " AddonStatusLoaded";
                break;
            case WebOSWindow.AddonStatusDenied:
                text += " AddonStatusDenied";
                break;
            case WebOSWindow.AddonStatusError:
                text += " AddonStatusError";
                break;
            }
            addonStatus.text = text;
        }
    }
}
