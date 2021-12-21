// Copyright (c) 2021 LG Electronics, Inc.
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

import QtQuick 2.0
import WebOS.DeveloperTools 1.0
import PresentationTime 1.0

Rectangle {
    id: root
    anchors.fill: parent
    visible: true

    Text {
        text: "DeliverUpdateRequest to Presentation"
        font.pointSize: 24
    }

    Repeater {
        model: [
           {"ms": 16, "color": "blue", "fontSize": 18},
           {"ms": 50, "color": "green", "fontSize": 18},
           {"ms": 100, "color": "yellow", "fontSize": 18},
        ]
        Rectangle {
            height: 1
            color: modelData.color
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: modelData.ms
            Text {
                text: modelData.ms + "ms"
                color: modelData.color
                font.pixelSize: modelData.fontSize
                anchors.bottom: parent.top
            }
        }
    }

    Graph {
        id: graph
        anchors.fill: parent
        z: 1000
        //TODO: enable custom mode
    }

    PresentationTime {
        onPresented: d2p => graph.addData(d2p/1000)
    }

    Rectangle {
        width: 50
        height: 50
        color:"red"
        anchors.right: parent.right
        RotationAnimation on rotation {
            from: 0; to: 360;
            duration: 10000
            loops: Animation.Infinite
        }
    }
}


