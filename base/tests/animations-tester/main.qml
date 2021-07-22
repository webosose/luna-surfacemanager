import QtQuick 2.4
import QtQuick.Window 2.2
import Eos.Window 0.1
import Eos.Controls 0.1
import QtGraphicalEffects 1.0

WebOSWindow {
    id: root
    width: 1920
    height: 1080
    visible: true
    title: "Various Animations Tester"
    color: "white"

    readonly property var animationList: [
        {"name":"Marquee_text", "url":"textscrolling.qml"},
        {"name":"Marquee_text2", "url":"textscrolling2.qml"},
        {"name":"Animated_image", "url":"animatedimage.qml"},
        {"name":"Rotate_image", "url":"rotateimage.qml"},
        {"name":"Rigid_transform_image", "url":"rigidtransformimage.qml"},
        {"name":"Scroll_page", "url":"scrollpage.qml"},
        {"name":"Slide_animation_page", "url":"slideanimationpage.qml"},
        {"name":"Wiper_page", "url":"wiperpage.qml"},
        {"name":"Mirroring_animation", "url":"mirroranimation.qml"}
    ]

    ListModel {
        id: animationModel
    }

    Loader {
        id: animationLoader
        anchors.fill: animationArea
    }

    Rectangle {
        id: background
        color: "transparent"
        anchors.fill: parent
        z: 1

        Text {
            id: infoText
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 100
            anchors.leftMargin: 100
            font.pixelSize: 64
            text: "Various Animations Tester"
        }

        Text {
            id: animationSelectorTitle
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 25
            anchors.rightMargin: 500
            horizontalAlignment: Text.AlignRight
            width: 400
            font.pixelSize: 48
            text: "Select Animation"
        }

        ComboBox {
            id: animationSelector
            anchors.right: background.right
            anchors.rightMargin: 100
            model: animationModel
            mustHighlight: true
            headerText: "Select Animation"
            focus: true
            onSelectedIndexChanged: {
                if (selectedIndex > 0 && selectedIndex <= animationList.length) {
                    animationsAllStop();
                    animationLoader.source = Qt.resolvedUrl("./animations/") + animationList[selectedIndex - 1].url;
                } else if (selectedIndex == 0) {
                    console.info("selectedIndex is 0, all animation stop.");
                    animationsAllStop();
                } else {
                    console.warn("selectedIndex is out of bounds:", selectedIndex);
                }
            }
        }
    }

    Rectangle {
        id: animationArea
        width: root.width
        height: root.height * 2 / 3
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        color: "transparent"
    }

    Component.onCompleted: {
        animationModel.clear();
        animationModel.append({"text": "0. Stop Animation"});
        for (var i = 0; i < animationList.length; i++)
            animationModel.append({"text": (i + 1) + ". " + animationList[i].name});
        animationSelector.selectedIndex = 0;
    }

    function animationsAllStop() {
        animationLoader.source = "";
    }
}
