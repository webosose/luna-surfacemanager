import QtQuick 2.0

Item {
    id: slideAnimationPage
    visible: true

    readonly property int slideInAnimationDuration: 3000

    Rectangle {
        id: pageOut
        color: "red"
        border.color: "black"
        border.width: 2
        width: animationArea.width * 2 / 3
        height: animationArea.height
        y: 0
        clip: true

        Text {
            id: pageOutText
            anchors.centerIn: parent
            text: "pageOut"
            font.pixelSize: 64
        }
    }

    Rectangle {
        id: pageIn
        color: "green"
        border.color: "black"
        border.width: 2
        width: animationArea.width * 2 / 3
        height: animationArea.height
        y: -animationArea.height
        clip: true

        Text {
            id: pageInText
            anchors.centerIn: parent
            text: "pageIn"
            font.pixelSize: 64
        }
    }

    ParallelAnimation {
        id: slideInAnimation
        running: true
        loops: Animation.Infinite
        NumberAnimation {
            target: pageIn
            property: "y"
            from: -animationArea.height
            to: 0
            easing.type: Easing.InOutCubic
            duration: slideAnimationPage.slideInAnimationDuration
        }
        NumberAnimation {
            target: pageIn
            property: "opacity"
            from: 0.0
            to: 1.0
            easing.type: Easing.Linear
            duration: slideAnimationPage.slideInAnimationDuration
        }
        NumberAnimation {
            target: pageOut
            property: "y"
            from: 0
            to: animationArea.height
            easing.type: Easing.InOutCubic
            duration: slideAnimationPage.slideInAnimationDuration
        }
        NumberAnimation {
            target: pageOut
            property: "opacity"
            from: 1.0
            to: 0.0
            easing.type: Easing.Linear
            duration: slideAnimationPage.slideInAnimationDuration
        }
    }
}
