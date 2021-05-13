import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    width: 1920
    height: 1080
    visible: true
    color: "yellow"
    windowType: "_WEBOS_WINDOW_TYPE_CARD"
    displayAffinity: params["displayAffinity"]
    title: "Multi Touch"

    Text {
        id: text0
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 100
        anchors.rightMargin: 100
        text: root.displayAffinity == 0 ? "This is a primary display" : "This is a secondary display"
        font.pixelSize: 32

        Component.onCompleted: forceActiveFocus();
        onActiveFocusChanged: {
            color = activeFocus ? "red" : "black";
        }
        Keys.onPressed: (event) => {
            text = text0.title + "\nKEY: " + event.key;
            cursor1.color = "red";
            switch (event.key) {
            case Qt.Key_Up:
                if (cursor1.y >= 20)
                    cursor1.y -= 20
                break;
            case Qt.Key_Down:
                if (cursor1.y <= root.height - 20)
                    cursor1.y += 20
                break;
            case Qt.Key_Left:
                if (cursor1.x >= 20)
                    cursor1.x -= 20
                break;
            case Qt.Key_Right:
                if (cursor1.x <= root.width - 20)
                    cursor1.x += 20
                break;
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onPositionChanged: (mouse) => {
             cursor1.x = mouse.x - cursor1.width/2;
             cursor1.y = mouse.y - cursor1.height/2;
        }
        onEntered: cursor1.color = "blue";
        onExited: cursor1.color = "gray";
        onPressed: (mouse) => { cursor1.color = "red"; }
        onReleased: (mouse) => { cursor1.color = "blue"; }
    }

    MultiPointTouchArea {
        anchors.fill: parent
        touchPoints: [
            TouchPoint {
                id: point1
                onXChanged: {
                    cursor1.x = x - cursor1.width/2;
                    cursor1.color = "blue";
                }
                onYChanged: {
                    cursor1.y = y - cursor1.height/2;
                    cursor1.color = "blue";
                }
            },
            TouchPoint {
                id: point2
                onXChanged: {
                    cursor2.x = x - cursor2.width/2;
                    cursor2.color = "blue";
                }
                onYChanged: {
                    cursor2.y = y - cursor2.height/2;
                    cursor2.color = "blue";
                }
            },
            TouchPoint {
                id: point3
                onXChanged: {
                    cursor3.x = x - cursor3.width/2;
                    cursor3.color = "blue";
                }
                onYChanged: {
                    cursor3.y = y - cursor3.height/2;
                    cursor3.color = "blue";
                }
            },
            TouchPoint {
                id: point4
                onXChanged: {
                    cursor4.x = x - cursor4.width/2;
                    cursor4.color = "blue";
                }
                onYChanged: {
                    cursor4.y = y - cursor4.height/2;
                    cursor4.color = "blue";
                }
            },
            TouchPoint {
                id: point5
                onXChanged: {
                    cursor5.x = x - cursor5.width/2;
                    cursor5.color = "blue";
                }
                onYChanged: {
                    cursor5.y = y - cursor5.height/2;
                    cursor5.color = "blue";
                }
            }
        ]
    }

    Rectangle {
        id: cursor1
        width: 20
        height: 20
        radius: 20
        color: "gray"
        Text {
            anchors.centerIn: parent
            text: "1"
        }
    }

    Rectangle {
        id: cursor2
        width: 20
        height: 20
        radius: 20
        color: "gray"
        Text {
            anchors.centerIn: parent
            text: "2"
        }
    }

    Rectangle {
        id: cursor3
        width: 20
        height: 20
        radius: 20
        color: "gray"
        Text {
            anchors.centerIn: parent
            text: "3"
        }
    }

    Rectangle {
        id: cursor4
        width: 20
        height: 20
        radius: 20
        color: "gray"
        Text {
            anchors.centerIn: parent
            text: "4"
        }
    }

    Rectangle {
        id: cursor5
        width: 20
        height: 20
        radius: 20
        color: "gray"
        Text {
            anchors.centerIn: parent
            text: "5"
        }
    }
}
