// Copyright (c) 2020-2024 LG Electronics, Inc.
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

Item {
    id: root
    anchors.fill: parent

    property int pointSize: 100
    property int cursorSize: 30
    property int pointMarginY: 80
    readonly property double opacityActive: 0.7
    property var colors: ["red", "gold", "purple", "cyan", "green"]

    Repeater {
        id: points
        model: colors

        Rectangle {
            width: pointSize
            height: width
            color: modelData
            radius: width/2
            visible: false
            opacity: opacityActive
        }
    }

    Rectangle {
        id: label

        width: 400
        height: column.height
        color: "black"

        opacity: 0.9
        visible: false

        anchors.top: parent.top
        anchors.right: parent.right

        Column {
            id: column
            spacing: 5

            Repeater {
                id: texts
                model: colors

                Text {
                    width: label.width
                    color: modelData
                    font.pixelSize: 16
                    wrapMode: Text.WrapAnywhere
                }
            }
        }
    }

    Item {
        id: touchHandler

        property var touchPoints: ({})

        Connections {
            target: compositorWindow
            function onDebugTouchUpdated(evt) {
                var touchPoints = evt.touchPoints;
                var removedTouchIds = [];
                for (var i = 0; i < touchPoints.length; i++) {
                    var tp = touchPoints[i];
                    if (tp.state == DebugTouchPoint.TouchPointReleased)
                        delete touchHandler.touchPoints[tp.touchId];
                    else
                        touchHandler.touchPoints[tp.touchId] = tp;
                }

                var touchIds = Object.keys(touchHandler.touchPoints);
                label.visible = touchIds.length > 0;

                var sortedIds = touchIds.sort();
                for (var i = 0 ; i < sortedIds.length && i < colors.length; i++) {
                    var pointItem = points.itemAt(i);
                    var touchPoint = touchHandler.touchPoints[sortedIds[i]];
                    var pos = touchHandler.mapFromItem(compositorWindow.contentItem,
                                                       touchPoint.pos.x, touchPoint.pos.y);
                    pointItem.visible = true;
                    pointItem.x = pos.x - pointSize / 2;
                    pointItem.y = pos.y - pointSize / 2;

                    var item = compositorWindow.itemAt(touchPoint.pos);
                    var text = texts.itemAt(i);
                    if (item) {
                        text.text = "TouchPoint " + +(i+1) + ": " + item.objectName;
                        text.visible = true;
                    } else {
                        text.visible = false;
                    }
                }
                for (var i = sortedIds.length; i < points.model.length && i < colors.length; i++) {
                    var pointItem = points.itemAt(i);
                    pointItem.visible = false;

                    var text = texts.itemAt(i);
                    text.visible = false;
                }
            }
        }
    }
}
