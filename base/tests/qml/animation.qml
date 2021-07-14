import QtQuick 2.0
import Eos.Window 0.1

WebOSWindow {
    id: root
    width: 1920
    height: 1080
    visible: true
    color: "white"

    Text {
        id: sampleText
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: -implicitWidth
        color: "black"
        font.pixelSize: 200
        text: "HELLO, WORLD"
    }

    SequentialAnimation {
        running: true
        PauseAnimation {
            duration: 3000
        }
        PropertyAnimation {
            target: sampleText
            running: true
            properties: "anchors.leftMargin"
            from: -sampleText.implicitWidth
            to: root.width
            duration: 10000
            loops: Animation.Infinite
        }
    }
}
