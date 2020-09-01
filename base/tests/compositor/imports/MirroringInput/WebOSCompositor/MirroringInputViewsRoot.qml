import QtQuick 2.4
import WebOSCompositorBase 1.0
import WebOSCoreCompositor 1.0

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

        Item {
            id: viewsRoot
            anchors.fill: parent

            property var foregroundItems: Utils.foregroundList(viewsRoot.children);
            property alias fullscreen: fullscreenViewId

            Rectangle {
                color: "steelblue"
                anchors.fill: parent
            }

            FullscreenView {
                id: fullscreenViewId
                anchors.fill: parent
                model: FullscreenWindowModel {}
            }

            SurfaceView {
                id: fullscreenClusterViewId
                anchors.fill: parent
                SurfaceItemMirror {
                    id: mirror
                    x: 0
                    y: 0
                    width: 0
                    height: 0
                    sourceItem: null
                    clustered: true
                    propagateEvents: true
                }
            }

            Rectangle {
                id: clusterButton
                x: 0
                y: 0
                width: 200
                height: 200
                border.color: "black"
                border.width: 5
                color: "white"
                radius: 5

                Text {
                    anchors.fill: parent
                    anchors.centerIn: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: "Toggle\nClustering"
                    color: "black"
                    font.pixelSize: 32
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (!mirror.sourceItem && !fullscreenViewId.currentItem) {
                            console.info("Failed to get currentItem");
                            return;
                        }

                        var windows = compositor.windowsInCluster(compositorWindow.clusterName, false);
                        if (!mirror.sourceItem) {
                            for (var i = 0; i < windows.length; i++)
                                windows[i].viewsRoot.openCluster(fullscreenViewId.currentItem);
                        } else {
                            for (var i = 0; i < windows.length; i++)
                                windows[i].viewsRoot.closeCluster();
                        }
                    }
                }
            }

            Rectangle {
                id: cloneButton
                x: 0
                y: 210
                width: 200
                height: 200
                border.color: "black"
                border.width: 5
                color: "white"
                radius: 5

                Text {
                    anchors.fill: parent
                    anchors.centerIn: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: "Toggle\nCloning"
                    color: "black"
                    font.pixelSize: 32
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (!mirror.sourceItem && !fullscreenViewId.currentItem) {
                            console.info("Failed to get currentItem");
                            return;
                        }

                        var windows = compositor.windowsInCluster(compositorWindow.clusterName);
                        if (!mirror.sourceItem) {
                            for (var i = 0; i < windows.length; i++)
                                windows[i].viewsRoot.openCluster(fullscreenViewId.currentItem, true);
                        } else {
                            for (var i = 0; i < windows.length; i++)
                                windows[i].viewsRoot.closeCluster();
                        }
                    }
                }
            }

            function openCluster(sourceItem, clone) {
                fullscreenClusterViewId.openView();
                mirror.sourceItem = sourceItem;
                mirror.clustered = !clone;
                if (clone) {
                    mirror.x = 0;
                    mirror.y = 0;
                    mirror.width = sourceItem.width;
                    mirror.height = sourceItem.height;
                } else {
                    mirror.x = -compositorWindow.positionInCluster.x;
                    mirror.y = -compositorWindow.positionInCluster.y;
                    mirror.width = compositorWindow.clusterSize.width;
                    mirror.height = compositorWindow.clusterSize.height;
                }
            }

            function closeCluster() {
                fullscreenClusterViewId.closeView();
                mirror.sourceItem = null;
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
