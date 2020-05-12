import QtQuick 2.4
import WebOSCompositorBase 1.0

Item {
    Rectangle {
        id: root

        anchors.centerIn: parent
        anchors.horizontalCenterOffset: compositorWindow.outputGeometry.x
        anchors.verticalCenterOffset: compositorWindow.outputGeometry.y
        width: compositorWindow.outputGeometry.width
        height: compositorWindow.outputGeometry.height
        rotation: compositorWindow.outputRotation
        clip: compositorWindow.outputClip

        color: "steelblue"

        Text {
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.horizontalCenter: parent.horizontalCenter
            color: "black"
            font.pixelSize: 64
            text: "Compositor Main Example"
        }

        Rectangle {
            id: fullscreenViewArea
            anchors.centerIn: parent
            width: compositorWindow.outputGeometry.width / 2
            height: compositorWindow.outputGeometry.height / 2
            color: "transparent"
            border.width: 4
            border.color: "blue"

            Text {
                anchors.centerIn: parent
                color: parent.border.color
                font.pixelSize: 32
                text: "FullscreenView"
            }
        }

        Item {
            id: viewsRoot
            anchors.fill: parent

            property var foregroundItems: Utils.foregroundList(viewsRoot.children);
            property alias fullscreen: fullscreenViewId

            FullscreenView {
                id: fullscreenViewId
                x: fullscreenViewArea.x
                y: fullscreenViewArea.y
                width: fullscreenViewArea.width
                height: fullscreenViewArea.height
                model: FullscreenWindowModel {}
            }
        }

        Binding {
            target: compositorWindow
            property: "viewsRoot"
            value: viewsRoot
        }
    }

    // Following are needed to make getForegroundAppInfo functional
    BaseLunaServiceAPI { appId: LS.appId; views: viewsRoot }
}
