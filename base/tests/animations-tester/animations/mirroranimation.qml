import QtQuick 2.4
import QtGraphicalEffects 1.0

Item {
    id: mirrorAnimation
    visible: true

    Rectangle {
        id: mirroredPage
        width: animationArea.width / 2
        height: animationArea.height
        border.width: 2
        border.color: "black"
        color: "blue"

        Text {
            id: mirroredPageText
            anchors.centerIn: parent
            text: "mirroredPage"
            font.pixelSize: 64
        }
    }

    DropShadow {
        id: mirroredShadow
        radius: 50
        samples: 50
        color: "#80000000"
        visible: true
        z: -1

        states: [
            State {
                name: "open"
                when: mirroredShadow.visible
                PropertyChanges {
                    target: mirroredShadow
                    source: mirroredPage
                    x: mirroredPage.x
                    y: mirroredPage.y
                    width: mirroredPage.width
                    height: mirroredPage.height
                }
            },
            State {
                name: "closed"
                when: !mirroredShadow.visible
                PropertyChanges {
                    target: mirroredShadow
                    source: null
                }
            }
        ]
    }

    Rectangle {
        id: mirroringPage
        width: animationArea.width / 2
        height: animationArea.height
        border.width: 2
        border.color: "black"
        color: "yellow"

        Text {
            id: mirroringPageText
            anchors.centerIn: parent
            text: "mirroringPage"
            font.pixelSize: 64
        }
    }

    DropShadow {
        id: mirroringShadow
        radius: 50
        samples: 50
        color: "#80000000"
        visible: true
        z: -1

        states: [
            State {
                name: "open"
                when: mirroringShadow.visible
                PropertyChanges {
                    target: mirroringShadow
                    source: mirroringPage
                    x: mirroringPage.x
                    y: mirroringPage.y
                    width: mirroringPage.width
                    height: mirroringPage.height
                }
            },
            State {
                name: "closed"
                when: !mirroringShadow.visible
                PropertyChanges {
                    target: mirroringShadow
                    source: null
                }
            }
        ]
    }

    SequentialAnimation {
        id: mirroringAnimation
        running: true
        loops: Animation.Infinite
        ParallelAnimation {
            PropertyAnimation {
                properties: "scale"
                targets: [mirroredPage, mirroringPage, mirroredShadow, mirroringShadow]
                from: 1
                to: 0.9
                duration: 1000
                easing { type: Easing.InQuad }
            }
            PropertyAnimation {
                properties: "verticalOffset, horizontalOffset"
                targets: [mirroredShadow, mirroringShadow]
                from: 0
                to: 50
                duration: 1000
                easing { type: Easing.InQuad }
            }
            PropertyAnimation {
                id: translationX
                properties: "x"
                targets: [mirroredPage, mirroringPage, mirroredShadow, mirroringShadow]
                duration: 1000
                easing { type: Easing.InBack; overshoot: 1 }
            }
            PropertyAnimation {
                id: translationY
                properties: "y"
                targets: [mirroredPage, mirroringPage, mirroredShadow, mirroringShadow]
                duration: 1000
                easing { type: Easing.InBack; overshoot: 1 }
            }
        }

        PropertyAnimation {
            targets: [mirroredPage, mirroredShadow]
            properties: "x"
            from: 0
            to: animationArea.width / 2
            duration: 1000
            easing.type: Easing.InOutCubic
        }

        NumberAnimation {
            properties: "scale"
            targets: [mirroredPage, mirroringPage, mirroredShadow, mirroringShadow]
            from: 0.9
            to: 1
            duration: 250
            easing { type: Easing.OutQuad }
        }

        PauseAnimation {
            duration: 500
        }

        PropertyAction {
            id: backToOriginX
            targets: [mirroredPage, mirroredShadow]
            properties: "x"
            value: 0
        }
    }
}
