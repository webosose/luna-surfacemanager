// Copyright (c) 2020 LG Electronics, Inc.
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

Rectangle {
    id: root
    visible: Settings.local.debug.enable && Settings.local.debug.surfaceHighlight === true && root.sourceItem && root.sourceView

    property Item sourceItem
    property Item sourceView
    property color hColor: __colors[root.sourceView ? root.sourceView.layerNumber % root.__colors.length : "white"]
    readonly property var __colors: [ "orangered", "red", "fuchsia", "crimson", "deeppink", "hotpink", "magenta" ]

    anchors.fill: parent
    color: "transparent"
    border.color: root.hColor
    border.width: 4

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        width: 400
        height: 100
        color: root.hColor
        opacity: 0.8
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
            color: "black"
            font.pixelSize: 16
            text: (root.sourceView ? "view:" + root.sourceView.objectName + "\n" : "") +
                  (root.sourceItem ? "appId: " + root.sourceItem.appId + "\n" +
                                    "type: " + root.sourceItem.type + "\n" +
                                    "size: " + root.sourceItem.width + "x" + root.sourceItem.height : "")
        }
    }
}
