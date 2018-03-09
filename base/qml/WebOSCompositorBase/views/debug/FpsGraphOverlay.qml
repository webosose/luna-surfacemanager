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
import WebOS.DeveloperTools 1.0
import WebOSCompositorBase 1.0

Item {
    id: root
    anchors.fill: parent
    property bool spinnerRepaint: Settings.local.debug.spinnerRepaint

    Rectangle {
        id: spinner
        width: 40
        height: 10
        color: "blue"
        visible: spinnerRepaint
        RotationAnimation on rotation {
            from: 0; to: 360;
            duration: 10000
            loops: Animation.Infinite
            running: spinnerRepaint
        }
    }

    Repeater {
        model: [
            {"ms": 16, "color": "blue", "fontSize": 18},
            {"ms": 50, "color": "green", "fontSize": 18},
            {"ms": 100, "color": "yellow", "fontSize": 18},
            {"ms": 500, "color": "orange", "fontSize": 18},
            {"ms": 1000, "color": "red", "fontSize": 18}
        ]
        Rectangle {
            height: 1
            color: modelData.color
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: modelData.ms
            Text {
                text: modelData.ms + "ms (" + (1000 / modelData.ms) + "fps)"
                color: modelData.color
                font.pixelSize: modelData.fontSize
                anchors.bottom: parent.top
            }
        }
    }

    FpsGraph {
        anchors.fill: parent
    }
}
