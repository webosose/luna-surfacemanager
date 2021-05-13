import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: root
    title: "Restricted Type"
    width: 1920
    height: 1080
    visible: true
    color: "pink"
    windowType: "_WEBOS_WINDOW_TYPE_RESTRICTED"
    displayAffinity: params["displayAffinity"]

    Text {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 100
        anchors.rightMargin: 100
        text: "WEBOS WINDOW TYPE - RESTRICTED"
        font.pixelSize: 32

        Component.onCompleted: forceActiveFocus();
        Keys.onPressed: (event) => {
            text = root.title + "\nKEY: " + event.key;
        }
        onActiveFocusChanged: {
            color = activeFocus ? "red" : "black";
        }
    }

    Rectangle {
        id: cursor
        width: 20
        height: 20
        radius: 20
        color: "gray"
        Text {
            anchors.centerIn: parent
            text: "R"
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onPositionChanged: (mouse) => {
             cursor.x = mouse.x - cursor.width/2;
             cursor.y = mouse.y - cursor.height/2;
        }
        onEntered: cursor.color = "blue";
        onExited: cursor.color = "gray";
        onPressed: (mouse) => { cursor.color = "red"; }
        onReleased: (mouse) => { cursor.color = "blue"; }
    }
}
