import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    title: "Overlay Type With VKB"
    width: 1920
    height: 1080
    visible: true
    color: "transparent"
    windowType: "_WEBOS_WINDOW_TYPE_OVERLAY"
    displayAffinity: params["displayAffinity"]

    Rectangle {
        color: "green"
        anchors.left: parent.left
        width: parent.width/2
        height: 1080
        opacity: 0.5

    Text {
            anchors.centerIn: parent
            text: "WEBOS WINDOW TYPE - OVERLAY"
            font.pixelSize: 32
            color: "red"
        }
    }

    Rectangle {
        id: text0
        anchors.top: parent.top
        anchors.left: parent.left
        width: parent.width/2
        height: 100
        anchors.topMargin: 50

        Text {
            id: label0
            anchors.left: parent.left
            anchors.verticalCenter : text0
            text: "Overlay : "
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 50
            width: 200
            height: 100
        }

        Rectangle {
            anchors.left: label0.right
            anchors.right: parent.right
            height: 100

            TextInput {
                id: input0
                font.pixelSize: 50
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                color: "blue"
                inputMethodHints: Qt.ImhNone
            }
        }
    }
}
