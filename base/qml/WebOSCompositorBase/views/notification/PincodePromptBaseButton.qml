// Copyright (c) 2013-2018 LG Electronics, Inc.
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
import WebOS.Global 1.0

Item {
    id: root

    property string text: ""

    property bool keyPressed: false
    property bool pressed: mouseArea.pressed || keyPressed
    property bool highlight : mouseArea.highlight

    signal clicked(string text)

    focus: false

    WebOSMouseArea {
        id: mouseArea
        cursorVisible: notificationRoot.cursorVisible
        anchors.fill: parent
        hoverEnabled: root.enabled
        onClicked: {
            if (root.enabled)
                root.clicked(root.text);
        }
        onPressed: {
            if (root.enabled) {
                root.focus = false;
                root.pressed = true;
            }
        }
        onReleased: {
            if (root.enabled) {
                root.focus = true;
                root.pressed = false;
            }
        }
        onHighlightChanged: {
            if (highlight && root.enabled)
                root.focus = true;
            else
                root.keyPressed = false;
        }
    }

    Keys.onPressed: {
        if (!root.enabled || event.isAutoRepeat)
            return;

        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            root.keyPressed = true;
        else
            root.keyPressed = false;
    }

    Keys.onReleased: {
        if (!root.enabled) return;

        if (root.keyPressed && event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.clicked(root.text);
            root.keyPressed = false;
        }
    }
}
