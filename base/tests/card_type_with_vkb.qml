import QtQuick 2.4
import Eos.Window 0.1
import Eos.VirtualKeyboardOverlay 0.1

WebOSWindow {
    id: root
    width: 1920
    height: 1080
    visible: true
    title: "Card Type With VKB"
    displayAffinity: params["displayAffinity"]

    Rectangle {
        id: border
        color: "lightsteelblue"
        anchors.fill: parent

        MouseArea {
            anchors.fill: parent
            onClicked: border.focus = true;
        }

        Rectangle {
            id: base
            color: "black"
            anchors.fill: parent
            anchors.margins: 10

            Text {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                color: "red"
                font.pointSize: 32
                text: "Click the black area to unfocus text input."
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 324
        anchors.bottom: parent.bottom
        color: "gray"
        border.color: "red"
        border.width: 2

        Text {
            text: "Default keyboard area"
            font.pointSize: 16
            color: "red"
        }
    }

    Rectangle {
        id: text0
        color: "white"
        x: 20
        y: 20
        width: parent.width - x * 2
        height: 50

        Text {
            id: label0
            anchors.left: parent.left
            text: "Normal: "
            font.pixelSize: 40
        }

        Rectangle {
            anchors.left: label0.right
            anchors.right: parent.right
            height: parent.height
            color: "lightsteelblue"

            TextInput {
                id: input0
                objectName: label0.text
                anchors.fill: parent
                font.pixelSize: 40
                color: "blue"
                inputMethodHints: Qt.ImhNone
            }
        }
    }

    VirtualKeyboardOverlay {
        target: input0
        anchors.top: text0.bottom
        width: 1920
        height: 324

        Rectangle {
            anchors.fill: parent
            color: "yellow"
            opacity: 0.2
        }

        Text {
            text: "VirtualKeyboardOverlay for Normal"
            font.pointSize: 16
            color: "red"
        }
    }

    Rectangle {
        id: text1
        color: "white"
        x: 20
        y: text0.y + 324 + 50 + 20
        width: parent.width - x * 2
        height: 50

        Text {
            id: label1
            anchors.left: parent.left
            text: "Digit: "
            font.pixelSize: 40
        }

        Rectangle {
            anchors.left: label1.right
            anchors.right: parent.right
            height: parent.height
            color: "lightsteelblue"

            TextInput {
                id: input1
                objectName: label1.text
                anchors.fill: parent
                font.pixelSize: 40
                color: "blue"
                inputMethodHints: Qt.ImhDigitsOnly
            }
        }
    }

    VirtualKeyboardOverlay {
        target: input1
        anchors.top: text1.bottom
        width: 1920
        height: 100

        Rectangle {
            anchors.fill: parent
            color: "yellow"
            opacity: 0.2
        }

        Text {
            text: "VirtualKeyboardOverlay for Digit"
            font.pointSize: 16
            color: "red"
        }
    }

    Rectangle {
        id: text2
        color: "white"
        x: 20
        y: text1.y + 100 + 50 + 20
        width: parent.width - x * 2
        height: 50

        Text {
            id: label2
            anchors.left: parent.left
            text: "URL(unset): "
            font.pixelSize: 40
        }

        Rectangle {
            anchors.left: label2.right
            anchors.right: parent.right
            height: parent.height
            color: "lightsteelblue"

            TextInput {
                id: input2
                objectName: label2.text
                anchors.fill: parent
                font.pixelSize: 40
                color: "blue"
                inputMethodHints: Qt.ImhUrlCharactersOnly
            }
        }
    }

    Rectangle {
        id: text3
        color: "white"
        x: 20
        y: text2.y + 50 + 20
        width: parent.width - x * 2
        height: 50

        Text {
            id: label3
            anchors.left: parent.left
            text: "Number(unset): "
            font.pixelSize: 40
        }

        Rectangle {
            anchors.left: label3.right
            anchors.right: parent.right
            height: parent.height
            color: "lightsteelblue"

            TextInput {
                id: input3
                objectName: label3.text
                anchors.fill: parent
                font.pixelSize: 40
                color: "blue"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
            }
        }
    }

    Rectangle {
        id: textX
        color: "white"
        x: 600
        y: text3.y + 50 + 20
        width: resizeHandle.x
        height: 50

        MouseArea {
            anchors.fill: parent
            drag.target: parent
            drag.axis: Drag.XAndYAxis
            drag.minimumX: 0
            drag.maximumX: 1900
        }

        Text {
            id: labelX
            anchors.left: parent.left
            text: "Email(draggable, resizable): "
            font.pixelSize: 40
        }

        Rectangle {
            anchors.left: labelX.right
            anchors.right: parent.right
            height: parent.height
            color: "lightsteelblue"

            TextInput {
                id: inputX
                objectName: labelX.text
                anchors.fill: parent
                font.pixelSize: 40
                color: "blue"
                inputMethodHints: Qt.ImhEmailCharactersOnly
            }
        }

        Rectangle {
            id: resizeHandle
            x: 800
            y: 0
            width: 50
            height: 50
            color: "blue"

            MouseArea {
                anchors.fill: parent
                drag.target: parent
                drag.axis: Drag.XAxis
                drag.minimumX: 0
                drag.maximumX: 1900
            }
        }
    }

    VirtualKeyboardOverlay {
        target: inputX
        anchors.top: textX.bottom
        x: textX.x
        width: textX.width
        height: 324

        Rectangle {
            anchors.fill: parent
            color: "yellow"
            opacity: 0.2
        }

        Text {
            text: "VirtualKeyboardOverlay for Email"
            font.pointSize: 16
            color: "red"
        }
    }
}
