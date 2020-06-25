import QtQuick 2.4

Rectangle {
    id: root
    anchors.fill: parent
    visible: true

    MultiPointTouchArea {
        anchors.fill: parent
        touchPoints: [
            TouchPoint { id: touchPoint }
        ]
    }

    Rectangle {
        id: redRect1
        x: 640
        y:360
        width: 640
        height: 360
        border.color: "red";
        border.width: 30

        Rectangle {
            id: redRect2
            x: 320
            y: 0
            width: 320
            height: 360
            border.color: "red";
            border.width: 30
        }
    }

    Rectangle {
        id: blackDot
        x: touchPoint.x
        y: touchPoint.y
        z: 1
        width: 20
        height: 20
        color: "black"
    }
}
