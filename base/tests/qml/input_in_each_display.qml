import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    width: 1920
    height: 1080
    visible: true
    title: "Input In Each Display"
    displayAffinity: params["displayAffinity"]

    Rectangle {
        id: rect
        color: "red"
        anchors.fill: parent

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onEntered: () => {
                text3.text = "onEntered";
            }
            onExited: () => {
                text3.text = "onExited";
            }
            onPressed: (mouse) => {
                text4.text = "onPressed";
            }
            onReleased: (mouse) => {
                text4.text = "onReleased";
            }
            onPositionChanged: (mouse) => {
                text5.text = "X = " + mouse.x + ", Y = " + mouseY
            }
            onClicked: (mouse) => {
                if (Qt.colorEqual(rect.color, "red"))
                    rect.color = "green";
                else
                    rect.color = "red";
            }
        }

        Text {
            id: text1
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -50
            text: root.displayAffinity == 0 ? "Primary Display" : "Secondary Display"
            font.pixelSize: 100
        }

        Text {
            id: text2
            focus: true
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 50
            font.pixelSize: 50
            text: "KEY:"
            Component.onCompleted: forceActiveFocus();
            Keys.onPressed: (event) => {
                text = "KEY: " + event.key;
            }
        }

        Text {
            id: text3
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 100
            font.pixelSize: 50
        }

        Text {
            id: text4
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 150
            font.pixelSize: 50
        }

        Text {
            id: text5
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 200
            font.pixelSize: 50
        }
    }
}
