import QtQuick 2.4
import Eos.Window 0.1
import TabletItem 1.0

WebOSWindow {
    id: root
    width: 1920
    height: 1080
    visible: true

    windowType: "_WEBOS_WINDOW_TYPE_CARD"

    property string label
    property string labelOther
    property string labelTouch
    property string firstUniqueId: ""

    Rectangle {
        width: 700
        height: 700
        color: "red"

        TabletItem {
            id: tabletItem
            anchors.fill: parent

            onMoved: {
                printOutput("Moved");
            }

            onPressed: {
                printOutput("Pressed");
            }

            onReleased: {
                printOutput("Released");
            }

            onTouchUpdated: {
                var output = "Touch Event\n";
                output += "Type: " + eventType + "\n";
                output += "x: " + xTouch + "\n";
                output += "y: " + yTouch + "\n";
                root.labelTouch = output;
            }

            function printOutput(str) {
                var uniqueIdString = Number(uniqueId).toString();
                if (firstUniqueId === "") {
                    firstUniqueId = uniqueIdString;
                    console.log('firstUniqueId: ' + firstUniqueId);
                }

                var output;
                output = "Tablet Event\n";
                output += "Device: " + device + "\n";
                output += "Type: " + type + "\n";
                output += "Action: " + str + "\n";
                output += "x: " + pos.x + "\n";
                output += "y: " + pos.y + "\n";
                output += "z: " + z + "\n";
                output += "xTilt: " + xTilt + "\n";
                output += "yTilt: " + yTilt + "\n";
                output += "Pressure: " + pressure + "\n";
                output += "UniqueId: " + uniqueId + "\n";

                if (uniqueIdString === firstUniqueId)
                    root.label = output;
                else
                    root.labelOther = output;
            }
        }
    }

    Text {
        id: myText
        anchors.top: parent.top
        anchors.topMargin: 40
        anchors.left: parent.left
        anchors.leftMargin: 40
        text: root.label
        font.pixelSize: 30
    }

    Text {
        id: textOther
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        anchors.left: parent.left
        anchors.leftMargin: 40
        text: root.labelOther
        font.pixelSize: 30
    }

    Text {
        id: textTouch
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        anchors.left: parent.left
        anchors.leftMargin: 500
        text: root.labelTouch
        font.pixelSize: 30
    }

    Rectangle {
        anchors.top: parent.top
        anchors.topMargin: 40
        anchors.right: parent.right
        anchors.rightMargin: 40
        width: 300
        height: 300
        color: "green"

        property int count

        Text {
            text: 'MouseArea'
            font.pixelSize: 30
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            anchors.left: parent.left
            anchors.leftMargin: 40
            id: mouseAreaClickStatus
            text: ''
            font.pixelSize: 30
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 80
            anchors.left: parent.left
            anchors.leftMargin: 40
            id: mouseAreaStatus
            text: ''
            font.pixelSize: 30
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 120
            anchors.left: parent.left
            anchors.leftMargin: 40
            id: mouseAreaX
            text: ''
            font.pixelSize: 30
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 160
            anchors.left: parent.left
            anchors.leftMargin: 40
            id: mouseAreaY
            text: ''
            font.pixelSize: 30
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                parent.count += 1;
                mouseAreaClickStatus.text = 'clicked ' + parent.count + ' times';
            }
            onPressed: {
                mouseAreaStatus.text = 'pressed';
            }
            onReleased: {
                mouseAreaStatus.text = 'released';
            }
            onPressAndHold: {
                mouseAreaStatus.text = 'pressAndHold';
                mouseAreaClickStatus.text = '';
            }
            onMouseXChanged: {
                mouseAreaX.text = "X:" + mouseX;
            }
            onMouseYChanged: {
                mouseAreaY.text = "Y:" + mouseY;
            }
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        anchors.right: parent.right
        anchors.rightMargin: 40
        width: 300
        height: 300
        color: "red"

        property int count

        Text {
            text: 'MultiPointTouchArea'
            font.pixelSize: 30
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 80
            anchors.left: parent.left
            anchors.leftMargin: 40
            id: touchStatus
            text: ''
            font.pixelSize: 30
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            anchors.left: parent.left
            anchors.leftMargin: 40
            id: touchUpdateStatus
            text: ''
            font.pixelSize: 30
        }

        MultiPointTouchArea {
            anchors.fill: parent
            onTouchUpdated: {
                parent.count += 1;
                touchUpdateStatus.text = 'touchUpdated ' + parent.count + ' times';
            }
            onPressed: {
                touchStatus.text = 'pressed';
            }
            onReleased: {
                touchStatus.text = 'released';
            }
        }
    }
}
