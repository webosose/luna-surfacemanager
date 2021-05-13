// Copyright (c) 2017-2021 LG Electronics, Inc.
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

Rectangle {
    id: root

    signal clicked()

    property bool keyPressed: false
    property bool pressed: closeMouseArea.pressed || keyPressed

    color: pressed ? Settings.local.notification.closeButton.foregroundColor : (closeMouseArea.highlight ? Settings.local.notification.closeButton.activatedColor : Settings.local.notification.closeButton.backgroundColor)
    border.color: pressed ? Settings.local.notification.closeButton.activatedColor : Settings.local.notification.closeButton.backgroundColor
    border.width: pressed ? width / 12 : 0
    width: Settings.local.notification.closeButton.size
    height: width
    radius: width * 2 / 3
    anchors.topMargin: Settings.local.notification.closeButton.margins
    anchors.rightMargin: Settings.local.notification.closeButton.margins
    smooth: true

    Keys.onPressed: (event) => {
        if (!event.isAutoRepeat) {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                keyPressed = true;
            else
                keyPressed = false;
        }
    }

    Keys.onReleased: (event) => {
        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && keyPressed) {
            keyPressed = false;
            root.clicked();
        }
    }

    Rectangle {
        id: slash
        color: root.pressed ? Settings.local.notification.closeButton.backgroundColor : Settings.local.notification.closeButton.foregroundColor
        height: parent.height * 0.5
        width: parent.height / 15
        rotation: 45
        anchors.centerIn: parent
        radius: parent.height / 20
        smooth: true
    }

    Rectangle {
        id: backSlash
        color: root.pressed ? Settings.local.notification.closeButton.backgroundColor : Settings.local.notification.closeButton.foregroundColor
        height: slash.height
        width: slash.width
        rotation: -45
        anchors.centerIn: parent
        radius: slash.radius
        smooth: true
    }

    WebOSMouseArea {
        id: closeMouseArea
        cursorVisible: notificationRoot.cursorVisible
        z: 100
        anchors.fill: parent
        propagateComposedEvents: true
        enabled: root.enabled
        hoverEnabled: enabled
        onClicked: (mouse) => { root.clicked(); }
        onHighlightChanged: {
            if (highlight)
                root.focus = true;
            else
                root.keyPressed = false;
        }
    }
}
