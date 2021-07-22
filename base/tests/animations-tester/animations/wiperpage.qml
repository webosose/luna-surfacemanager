import QtQuick 2.4

Item {
    id: wiperPage
    visible: true

    Rectangle {
        id: backgroundPage
        width: animationArea.width * 2 / 3
        height: animationArea.height
        border.width: 2
        border.color: "black"
        color: "yellow"

        Text {
            id: backgroundPageText
            anchors.centerIn: parent
            text: "backgroundPage"
            font.pixelSize: 64
        }
    }

    Rectangle {
        id: wiper
        width: 100
        height: animationArea.height
        color: "lightgray"
    }

    Canvas {
        id: wiperCanvas
        width: 400
        anchors.left: wiper.right
        height: animationArea.height
        contextType: "2d"

        readonly property int curvature: 100

        Path {
            id: wiperCurve
            startY: 0

            PathCurve { relativeX: wiperCanvas.curvature; relativeY: animationArea.height / 2 }
            PathCurve { relativeX: -wiperCanvas.curvature; relativeY: animationArea.height / 2 }
        }

        onPaint: {
            context.strokeStyle = "lightgray"
            context.fillStyle = "lightgray"
            context.lineWidth = 2;
            context.path = wiperCurve
            context.stroke();
            context.fill();
        }
    }

    SequentialAnimation {
        id: wiperAnimation
        running: true
        loops: Animation.Infinite
        PropertyAnimation {
            target: wiper
            property: "width"
            from: 10
            to: animationArea.width * 2 / 3 - wiperCanvas.curvature
            duration: 2000
        }
    }
}
