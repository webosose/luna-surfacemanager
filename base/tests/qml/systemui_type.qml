import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    title: "WEBOS WINDOW TYPE - SYSTEM_UI"
    width: 600
    height: 600
    visible: true
    color: "transparent"
    windowType: "_WEBOS_WINDOW_TYPE_SYSTEM_UI"
    displayAffinity: params["displayAffinity"]

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        width: 600
        height: 600
        color: "green"
        opacity: 0.5

        Text {
            anchors.centerIn: parent
            text: root.title
            font.pixelSize: 32
            color: "red"
            Component.onCompleted: forceActiveFocus();
            Keys.onPressed: (event) => {
                text = root.title + "\nKEY: " + event.key;
            }
            onActiveFocusChanged: {
                color = activeFocus ? "red" : "black";
            }
        }
    }

    Rectangle {
        id: cursor
        width: 20
        height: 20
        radius: 20
        color: "red"
        Text {
            anchors.centerIn: parent
            text: "O"
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onPositionChanged: (mouse) => {
             cursor.x = mouse.x - cursor.width/2;
             cursor.y = mouse.y - cursor.height/2;
        }
    }
}
