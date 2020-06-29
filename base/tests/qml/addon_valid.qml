import QtQuick 2.4
import WebOSCompositorBase 1.0

Rectangle {
    x: 50
    y: 50
    width: 900
    height: 300
    radius: 10
    color: "olive"

    Text {
        anchors.centerIn: parent
        font.pixelSize: 30
        text: "addon_valid.qml" + "\n\n" + Settings.local.addon.directories.join("\n")
    }
}
