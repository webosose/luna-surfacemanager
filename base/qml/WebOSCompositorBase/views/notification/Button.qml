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
import WebOSCompositorBase 1.0

FocusScope {
    id: root

    property string text: ""

    property bool keyPressed: false
    property bool pressed: mouseArea.pressed || keyPressed

    signal clicked

    width: Settings.local.notification.button.width
    height: Settings.local.notification.button.height

    Rectangle {
        id: background
        anchors.fill: parent
        color: (mouseArea.highlight && !root.pressed) ? Settings.local.notification.button.activatedColor : Settings.local.notification.button.backgroundColor
        border.color: root.pressed ? Settings.local.notification.button.activatedColor : Settings.local.notification.button.backgroundColor
        border.width: root.pressed ? 5 : 0
        radius: height * 0.7
        smooth: true
    }

    MarqueeText {
        id: buttonText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        text: root.text
        color: mouseArea.highlight && !root.pressed ? Settings.local.notification.button.textActivatedColor : Settings.local.notification.button.textColor
        font.family: Settings.local.notification.button.fontName
        font.pixelSize: Settings.local.notification.button.fontSize
        font.capitalization: Font.AllUppercase
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        width: root.width - root.height * 0.5
        marqueeEnabled: mouseArea.highlight
    }

    WebOSMouseArea {
        id: mouseArea
        cursorVisible: notificationRoot.cursorVisible
        anchors.fill: background
        hoverEnabled: true
        onClicked: root.clicked();
        onHighlightChanged: {
            if (highlight)
                root.focus = true;
            else
                root.keyPressed = false;
        }
    }

    Keys.onPressed: {
        if (!event.isAutoRepeat) {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                root.keyPressed = true;
            else
                root.keyPressed = false;
        }
    }

    Keys.onReleased: {
        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && root.keyPressed) {
            root.keyPressed = false;
            root.clicked();
        }
    }
}
