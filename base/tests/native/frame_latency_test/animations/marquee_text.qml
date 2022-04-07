// Copyright (c) 2022 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); // you may not use this file except in compliance with the License.
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

Item {
    id: marqueeTextAnimation
    width: animationArea.width
    height: animationArea.height
    visible: true

    Text {
        id: sampleText
        color: "black"
        font.pixelSize: 100
        horizontalAlignment: Text.AlignRight
        text: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" + "\n" +
              "abcdefghijklmnopqrstuvwxyzabcdefg" + "\n" +
              "012345678901234567890123456789"
    }

    SequentialAnimation {
        id: scrollAnimation
        running: true
        loops: Animation.Infinite
        PropertyAnimation {
            target: sampleText
            properties: "x"
            from: -sampleText.implicitWidth
            to: animationArea.width
            duration: 10000
        }
    }
}
