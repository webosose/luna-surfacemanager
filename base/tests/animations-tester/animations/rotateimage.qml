import QtQuick 2.4

Item {
    id: rotateImage
    visible: true

    Image {
        id: sampleImage
        width: 640
        height: 640
        source: Qt.resolvedUrl("../resources/sample.png")
        anchors.centerIn: parent
        fillMode: Image.PreserveAspectFit
    }

    Rectangle {
        id: rigidBox
        width: 320
        height: 80
        color: "black"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 100
    }

    SequentialAnimation {
        id: rotationAnimatorImage
        running: true
        loops: Animation.Infinite
        PropertyAnimation {
            target: sampleImage
            properties: "rotation"
            from: 0
            to: 360
            duration: 10000
        }
    }


    ParallelAnimation {
        id: multipleAnimator
        running: true
        loops: Animation.Infinite
        RotationAnimator {
            id: rotationAnimator
            target: rigidBox
            from: 0
            to: 360
            duration: 10000
        }
        ScaleAnimator {
            id: scaleAnimator
            target: rigidBox
            from: 1
            to: 0.8
            duration: 10000
        }
        OpacityAnimator {
            id: opacityAnimator
            target: rigidBox
            from: 1
            to: 0.5
            duration: 10000
        }
    }
}
