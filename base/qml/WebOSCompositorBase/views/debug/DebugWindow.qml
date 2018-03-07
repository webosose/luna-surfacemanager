// Copyright (c) 2014-2018 LG Electronics, Inc.
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

Rectangle {
    id: root
    x: Settings.local.debug.debugWindow.x
    y: Settings.local.debug.debugWindow.y
    width: Settings.local.debug.debugWindow.width
    height: Settings.local.debug.debugWindow.height
    color: dragArea.drag.active ? "lightgreen" : "lightsteelblue"
    opacity: Settings.local.debug.debugWindow.opacity
    property string title
    property string statusText
    property Item mainItem
    property real statusbarHeight: 20
    signal selected(Item item)

    onMainItemChanged: {
        if (mainItem) {
            mainItem.parent = view;
            mainItem.anchors.fill = view;
        }
    }

    Rectangle {
        id: titlebar
        z: 10
        color: "white"
        height: 20
        width: parent.width

        Text {
            text: root.title
            font.pixelSize: 18
        }

        MouseArea {
            id: dragArea
            anchors.fill: parent
            drag.target: root
            onClicked: {
                if (statusbarHeight)
                    if (root.height <= 20)
                        root.height = 400;
                    else
                        root.height = 20;
            }
            onPressed: root.selected(dragArea);
        }
    }

    Item {
        id: view
        clip: true
        anchors {
            left: parent.left
            right: parent.right
            top: titlebar.bottom
            bottom: statusbar.top
        }

        MouseArea {
            anchors.fill: parent
            onPressed: root.selected(view);
        }
    }

    Rectangle {
        id: statusbar
        color: "gray"
        height: root.statusbarHeight
        width: parent.width
        anchors.bottom: parent.bottom

        Text {
            text: root.statusText
            font.pixelSize: 18
        }

        MouseArea {
            anchors.fill: parent
            onPressed: root.selected(statusbar);
        }
    }

    Rectangle {
        id: resizeHandle
        width: 20
        height: root.statusbarHeight
        color: resizeArea.drag.active? "lightgreen" : "white"
        anchors.bottom: resizeArea.drag.active ? undefined : root.bottom
        anchors.right: resizeArea.drag.active ? undefined : root.right

        MouseArea {
            id: resizeArea
            anchors.fill: resizeHandle
            drag.target: resizeHandle
            onPositionChanged: {
                if (drag.active) {
                    root.width = Math.max(100, resizeHandle.x + resizeHandle.width);
                    root.height = Math.max(20, resizeHandle.y + resizeHandle.height);
                }
            }
            onPressed: root.selected(resizeHandle);
        }
    }

    Behavior on height {
        enabled: !resizeArea.drag.active
        NumberAnimation {}
    }
}
