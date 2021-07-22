import QtQuick 2.4

Item {
    id: rotateImage
    visible: true

    Image {
        id: sampleImage
        width: 480
        height: 320
        source: Qt.resolvedUrl("../resources/sample.png")
        fillMode: Image.PreserveAspectFit
    }

    SequentialAnimation {
        id: rotationAnimatorImage
        running: true
        loops: Animation.Infinite
        ParallelAnimation {
            PropertyAnimation {
                target: sampleImage
                properties: "x"
                from: 0
                to: (animationArea.width - sampleImage.width) / 3
                duration: 2500
            }
            PropertyAnimation {
                target: sampleImage
                properties: "rotation"
                from: 0
                to: 360
                duration: 2500
            }
        }
        PauseAnimation {
            duration: 500
        }
        ParallelAnimation {
            PropertyAnimation {
                target: sampleImage
                properties: "y"
                from: 0
                to: animationArea.height - sampleImage.height
                duration: 2500
            }
            PropertyAnimation {
                target: sampleImage
                properties: "rotation"
                from: 0
                to: 360
                duration: 2500
            }
        }
        PauseAnimation {
            duration: 500
        }
        ParallelAnimation {
            PropertyAnimation {
                target: sampleImage
                properties: "x"
                from: (animationArea.width - sampleImage.width) / 3
                to: animationArea.width - sampleImage.width
                duration: 2500
            }
            PropertyAnimation {
                target: sampleImage
                properties: "y"
                from: animationArea.height - sampleImage.height
                to: 0
                duration: 2500
            }
            PropertyAnimation {
                target: sampleImage
                properties: "rotation"
                from: 0
                to: 360
                duration: 2500
            }
        }
        PauseAnimation {
            duration: 500
        }
        PropertyAction {
            id: backToOrigin
            target: sampleImage
            properties: "x, y"
            value: 0
        }
    }
}
