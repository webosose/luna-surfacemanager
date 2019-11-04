import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    title: "Popup Type"
    width: 500
    height: 500
    visible: true
    color: "transparent"
    windowType: "_WEBOS_WINDOW_TYPE_POPUP"
    displayAffinity: params["displayAffinity"]

    Rectangle {
        anchors.fill: parent
        color: "blue"
        opacity: 0.5

        Text {
            id: text0
            anchors.centerIn: parent
            text: "WEBOS WINDOW TYPE - POPUP"
            font.pixelSize: 32
            color: "red"
            Component.onCompleted: forceActiveFocus();
            Keys.onPressed: {
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
        onPositionChanged: {
             pointerCursor.x = mouse.x - pointerCursor.width/2;
             pointerCursor.y = mouse.y - pointerCursor.height/2;
        }
        onEntered: pointerCursor.color = "steelblue";
        onExited: pointerCursor.color = "gray";
        onPressed: pointerCursor.color = "blue";
        onReleased: pointerCursor.color = "steelblue";
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
