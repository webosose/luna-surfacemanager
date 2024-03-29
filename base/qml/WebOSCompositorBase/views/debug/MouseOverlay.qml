// Copyright (c) 2023 LG Electronics, Inc.
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

MouseArea {
    id: mouseArea
    propagateComposedEvents: true
    property int xPos
    property int yPos

    anchors.fill: parent
    onPressed:(mouse) => { xPos=mouse.x; yPos=mouse.y; mouse.accepted = false; }

    Rectangle {
        id: textBox
        color:"yellow"
        x: mouseArea.xPos
        y: mouseArea.yPos
        width: 100
        height: 20

        Text {
            id: text
            anchors.fill: parent
            text: "(" + textBox.x +"," + textBox.y + ")"
        }
    }
}
