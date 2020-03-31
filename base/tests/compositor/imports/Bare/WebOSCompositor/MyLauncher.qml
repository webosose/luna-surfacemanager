import QtQuick 2.4
import WebOSCompositorBase 1.0
import WebOSCompositor 1.0

Launcher {
    id: root

    Text {
        anchors.centerIn: parent
        text: "MY LAUNCHER"
        opacity: 0.5
        color: "yellow"
        font.pixelSize: 128
        style: Text.Outline
        styleColor: "white"
    }
}
