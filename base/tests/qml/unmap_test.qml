import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    title: "Test surfaceUnmapped by setting the window invisible"
    width: 1920
    height: 1080
    visible: true
    color: "yellow"
    windowType: "_WEBOS_WINDOW_TYPE_CARD"

    property int countdown: 10

    Text {
        id: label
        anchors.centerIn: parent
        text: "Invisible in " + countdown + " second(s)"
        font.pixelSize: 100
    }

    Timer {
        id: timer
        interval: 1000
        repeat: true
        running: true
        onTriggered: {
            if (countdown == 0) {
                root.visible = false;
                stop();
            }
            countdown = countdown - 1;
        }
    }
}
