// Copyright (c) 2021-2022 LG Electronics, Inc.
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
import Eos.Controls 0.1
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
           {"ms": 16, "color": "blue", "fontSize": 15},
           {"ms": 32, "color": "green", "fontSize": 15},
           {"ms": 48, "color": "yellow", "fontSize": 15},
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
        id: background
        color: "transparent"
        anchors.fill: parent
        z: 1

        Text {
            id: animationSelectorTitle
            anchors.top: animationSelector.top
            anchors.topMargin: 10
            anchors.right: animationSelector.left
            anchors.rightMargin: 30
            horizontalAlignment: Text.AlignRight
            width: 400
            font.pixelSize: 24
            text: "Select Animation"
        }

        ComboBox {
            id: animationSelector
            anchors.top: parent.top
            anchors.topMargin: 25
            anchors.right: parent.right
            anchors.rightMargin: 20
            model: animationModel
            mustHighlight: true
            focus: true
            Component.onCompleted: {
                // Override styles
                style.comboWidth = 400;
                style.comboItemHeight = 46;
                style.comboVerticalMargin = 0;
                style.comboHorizontalMargin = 20;
                style.comboCornerRadius = 10;
                style.comboHeaderFontSize = 24;
                style.comboHeaderHeight = 46;
                style.comboItemFontSize = 24;
            }
            onSelectedIndexChanged: {
                if (selectedIndex >= 0 && selectedIndex < animationList.length) {
                    animationsAllStop();
                    animationLoader.source = Qt.resolvedUrl("./animations/") + animationList[selectedIndex].url;
                } else {
                    console.warn("selectedIndex is out of bounds:", selectedIndex);
                }
            }
        }
    }

    readonly property var animationList: [
        {"name":"Rotate_rect", "url":"rotate_rect.qml"},
        {"name":"Marquee_text", "url":"marquee_text.qml"},
    ]

    ListModel {
        id: animationModel
    }

    Loader {
        id: animationLoader
        anchors.fill: animationArea
    }

    Rectangle {
        id: animationArea
        width: root.width
        height: root.height * 2 / 3
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 100
        color: "transparent"
    }

    Component.onCompleted: {
        animationModel.clear();
        for (var i = 0; i < animationList.length; i++)
            animationModel.append({"text": (i + 1) + ". " + animationList[i].name});
        animationSelector.selectedIndex = 0;
    }

    function animationsAllStop() {
        animationLoader.source = "";
    }
}


