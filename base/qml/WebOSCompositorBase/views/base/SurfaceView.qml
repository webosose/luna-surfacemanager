// Copyright (c) 2017-2018 LG Electronics, Inc.
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
import WebOSCoreCompositor 1.0
import WebOSCompositorBase 1.0

import "qrc:/WebOSCompositor"
import "file:///usr/lib/qml/WebOSCompositor"

FocusableView {
    id: root
    hasForegroundItem: true

    property WindowModel model
    property bool fill: false
    property bool positioning: false
    property bool consumeMouseEvents: false
    property bool focusOnSurface: true
    property Item currentItem

    signal surfaceTransformUpdated(Item item)
    signal surfaceAdded(Item item)
    signal surfaceRemoved(Item item)

    // For surface group
    property bool grouped: false
    property var groupedItems: []
    signal groupFocused

    Connections {
        target: model

        onSurfaceAdded: {
            console.log("Adding item " + item + " to " + root);
            if (root.fill) {
                item.scale = Qt.binding(
                        function() {
                            return item.rotation % 180 ? Math.min(root.width / item.height, root.height / item.width) : Math.min(root.width / item.width, root.height / item.height); });
            }

            if (root.positioning) {
                // Align to center
                item.x = Qt.binding(function() { return Utils.center(root.width, item.width); });
                item.y = Qt.binding(function() { return Utils.center(root.height, item.height); });
            }

            root.surfaceTransformUpdated(item);
            root.surfaceAdded(item);
            Utils.performanceLog.timeEnd("APP_LAUNCH", {"APP_ID": item.appId});
            root.contentChanged();
            console.log("Item added in " + root + ", currentItem: " + currentItem);
        }

        onSurfaceRemoved: {
            console.log("Removing item " + item + " from " + root);
            root.surfaceRemoved(item);
            root.contentChanged();
            console.log("Item removed from " + root + ", currentItem: " + currentItem);
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.visible && root.consumeMouseEvents
        hoverEnabled: enabled
        visible: enabled
        acceptedButtons: Qt.AllButtons
        onWheel: {}
    }

    onFocused: {
        if (grouped) {
            root.groupFocused();
        } else if (activeFocus && root.currentItem) {
            if (root.focusOnSurface)
                root.currentItem.takeFocus();
            else
                console.warn("focusOnSurface is unset, skip setting focus on the current surface");
        }
    }

    Loader {
        active: Settings.local.debug.enable && Settings.local.debug.surfaceHighlight === true && root.currentItem
        visible: active
        z: 10000

        sourceComponent: Component {
            Rectangle {
                id: debugHighlight

                property color hColor: __colors[root.layerNumber % __colors.length]
                property var __colors: [ "orangered", "red", "fuchsia", "crimson", "deeppink", "hotpink", "magenta" ]

                property rect hRect: root.currentItem.mapToItem(root, 0, 0, root.currentItem.width, root.currentItem.height)

                x: hRect.x
                y: hRect.y
                width: hRect.width
                height: hRect.height
                color: "transparent"
                border.color: hColor
                border.width: 4

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    width: 400
                    height: 100
                    color: debugHighlight.hColor
                    opacity: 0.8
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.top: parent.top
                        anchors.topMargin: 10
                        color: "black"
                        font.pixelSize: 16
                        text: "view: " + root.objectName + "\n" +
                              "appId: " + root.currentItem.appId + "\n" +
                              "type: " + root.currentItem.type + "\n" +
                              "geometry: (" + debugHighlight.hRect.x + ", " + debugHighlight.hRect.y + ") " + debugHighlight.hRect.width + "x" + debugHighlight.hRect.height
                    }
                }
            }
        }
    }
}
