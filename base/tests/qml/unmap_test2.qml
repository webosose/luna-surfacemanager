import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    title: "Test surfaceMapped right after surfaceUnmapped"
    width: 1920
    height: 1080
    visible: true
    color: "yellow"
    windowType: "_WEBOS_WINDOW_TYPE_CARD"

    Text {
        id: label
        anchors.centerIn: parent
        text: root.visible ? "VISIBLE" : "NOT VISIBLE"
        font.pixelSize: 100
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (mouse) => {
            root.visible = false;
            timer.restart();
        }
    }

    Timer {
        id: timer
        interval: 200
        repeat: false
        running: false
        onTriggered: {
            root.visible = true;
        }
    }
}
