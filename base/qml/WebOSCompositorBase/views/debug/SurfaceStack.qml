// Copyright (c) 2020-2021 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

import QtQuick 2.4
import WebOSCompositorBase 1.0
import WebOSCoreCompositor 1.0

import "DebugUtils.js" as DebugUtils

ListView {
    id: root

    property real scaleFactor: 0.3
    property real rotationDegree: 65 // less than 90
    property real borderWidth: 4
    property real labelHeight: 30
    property color labelColor: "black"
    property int animationDuration: 400

    property real scaledWidth: compositorWindow.outputGeometry.width * scaleFactor + borderWidth * 2
    property real scaledHeight: compositorWindow.outputGeometry.height * scaleFactor + labelHeight + borderWidth * 2
    property real projectedWidth: scaledWidth * (1 - Math.cos(Math.PI * rotationDegree / 180))

    enabled: false
    width: compositorWindow.outputGeometry.width
    height: scaledHeight * 1.2
    x: -projectedWidth / 2 + 100
    orientation: ListView.Horizontal
    spacing: -projectedWidth * 0.9

    model: ListModel {
        id: surfaceModel

        Component.onCompleted: {
            update();
        }

        function update() {
            var matchedIndices = [];
            // Remove deleted items
            for (var i = count - 1; i >= 0; i--) {
                var j = compositorWindow.viewsRoot.foregroundItems.indexOf(get(i).currentItem);
                if (j < 0)
                    remove(i, 1);
                else
                    matchedIndices.push(j);
            }
            // Add new items in order
            for (var i = 0; i < compositorWindow.viewsRoot.foregroundItems.length; i++) {
                if (matchedIndices.indexOf(i) < 0) {
                    insert(i, {"currentItem": compositorWindow.viewsRoot.foregroundItems[i]})
                } else {
                    for (var j = 0; j < count; j++) {
                        if (compositorWindow.viewsRoot.foregroundItems[i] == get(j).currentItem) {
                            move(j, i, 1)
                            break;
                        }
                    }
                }
            }
        }
    }

    delegate: Component {
        Rectangle {
            width: scaledWidth
            height: scaledHeight
            opacity: 0.8
            color: "transparent"
            border.width: borderWidth
            border.color: DebugUtils.colorByType(currentItem)

            SurfaceItemMirror {
                anchors.top: parent.top
                anchors.topMargin: borderWidth + labelHeight
                anchors.left: parent.left
                anchors.leftMargin: borderWidth
                width: currentItem.width * scaleFactor
                height: currentItem.height * scaleFactor
                sourceItem: currentItem
            }

            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: borderWidth
                anchors.left: parent.left
                anchors.leftMargin: borderWidth
                anchors.right: parent.right
                anchors.rightMargin: borderWidth
                height: labelHeight
                z: 100
                color: DebugUtils.colorByType(currentItem)
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: borderWidth
                    anchors.verticalCenter: parent.verticalCenter
                    color: labelColor
                    font.pixelSize: parent.height / 1.5
                    text: currentItem.appId + " (" + currentItem.width + "x" + currentItem.height + ")"
                }
            }

            transform: Rotation {
                origin.x: width / 2
                origin.y: height / 2
                axis { x: 0; y: 1; z: 0 }
                angle: rotationDegree
            }
        }
    }

    add: Transition {
        NumberAnimation {
            properties: "y"
            from: root.height * 1.5
            duration: animationDuration
            easing.type: Easing.OutCubic
        }
    }

    remove: Transition {
        NumberAnimation {
            properties: "y"
            to: root.height * 1.5
            duration: animationDuration
            easing.type: Easing.OutCubic
        }
    }

    move: Transition {
        NumberAnimation {
            properties: "x"
            duration: animationDuration
            easing.type: Easing.OutCubic
        }
    }

    displaced: move

    Connections {
        target: compositorWindow.viewsRoot
        function onForegroundItemsChanged() {
            surfaceModel.update();
        }
    }
}
