import QtQuick 2.4
import QtQuick.Window 2.2
import Eos.Window 0.1
import Eos.Controls 0.1

WebOSWindow {
    id: root
    width: 1920
    height: 1080
    visible: true
    title: "Variable Display Affinity"

    ListModel {
        id: displayModel
    }

    Rectangle {
        id: rect
        color: "orange"
        anchors.fill: parent

        Text {
            id: infoText
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 100
            anchors.leftMargin: 100
            font.pixelSize: 64
            text: "Display ID: " + root.displayAffinity + "\n" +
                  "Screen size: " + Screen.width + "x" + Screen.height + "\n" +
                  "Screen name: " + Screen.name + "\n" +
                  "Screen orientation: " + Screen.orientation + "\n" +
                  "Device pixel ratio: " + Screen.devicePixelRatio + "\n" +
                  "Window size: " + Window.width + "x" + Window.height + "\n" +
                  "Actual window size: " + Window.width * Screen.devicePixelRatio + "x" + Window.height * Screen.devicePixelRatio
        }

        Text {
            id: displaySelectorTitle
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 100
            anchors.rightMargin: 100
            horizontalAlignment: Text.AlignRight
            width: 400
            font.pixelSize: 64
            text: "Select Display ID"
        }

        ComboBox {
            id: displaySelector
            anchors.top: displaySelectorTitle.bottom
            anchors.right: displaySelectorTitle.right
            model: displayModel
            mustHighlight: true
            onSelectedIndexChanged: {
                if (selectedIndex >= 0 && selectedIndex < Qt.application.screens.length)
                    root.displayAffinity = displaySelector.selectedIndex;
                else
                    console.warn("selectedIndex is out of bounds:", selectedIndex, Qt.application.screens.length);
            }
        }
    }

    onWindowStateChanged: {
        if (windowState == Qt.WindowFullScreen) {
            displayModel.clear();
            for (var i = 0; i < Qt.application.screens.length; i++)
                displayModel.append({"text": "Display " + i});
            displaySelector.selectedIndex = root.displayAffinity;
        } else {
            displayModel.clear();
        }
    }
}
