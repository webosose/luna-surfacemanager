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
    id: fpsGraphOverlay
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

    Rectangle {
        height: 20
        color: "green"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        Text { text: "16ms (60fps)"; color: "white"; font.pixelSize: 18; }
    }

    Rectangle {
        height: 1
        color: "yellow"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 100
        Text { text: "100ms (10fps)"; color: "yellow"; font.pixelSize: 18; }
    }

    Rectangle {
        height: 1
        color: "red"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 500
        Text { text: "500ms"; color: "red"; font.pixelSize: 18; }
    }

    Rectangle {
        id: flasher
        color: "red"
        opacity: .5
        visible: false
        anchors.fill: parent

        Timer {
            id: hideTimer
            running: false
            interval: 1000 / 60 // one frame
            repeat: false
            onTriggered: flasher.visible = false;
        }
    }

    FpsGraph {
        anchors.fill: parent
    }
}
