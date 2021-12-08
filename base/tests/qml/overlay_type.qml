import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    title: "WEBOS WINDOW TYPE - OVERLAY"
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
            id: text0
            anchors.centerIn: parent
            text: root.title
            font.pixelSize: 32
            color: "red"
            Component.onCompleted: forceActiveFocus();
            Keys.onPressed: (event) => {
                text = root.title + "\nKEY: " + event.key;
            }
        }
    }

    Rectangle {
        id: pointerCursor
        width: 20
        height: 20
        radius: 20
        color: "gray"
        Text {
            anchors.centerIn: parent
            text: "P"
        }
    }

    Rectangle {
        id: touchCursor
        width: 20
        height: 20
        radius: 20
        color: "red"
        Text {
            anchors.centerIn: parent
            text: "T"
        }

        x: point1.x
        y: point1.y
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onPositionChanged: (mouse) => {
             pointerCursor.x = mouse.x - pointerCursor.width/2;
             pointerCursor.y = mouse.y - pointerCursor.height/2;
        }
        onEntered: pointerCursor.color = "steelblue";
        onExited: pointerCursor.color = "gray";
        onPressed: (mouse) => { pointerCursor.color = "blue"; }
        onReleased: (mouse) => { pointerCursor.color = "steelblue"; }
    }

    MultiPointTouchArea {
        anchors.fill: parent
        touchPoints: [
            TouchPoint {
                id: point1
                onPressedChanged: {
                    text0.color = pressed ? "red" : "black";
                }
            }
        ]
    }
}
