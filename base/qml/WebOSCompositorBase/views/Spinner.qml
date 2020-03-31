// Copyright (c) 2017-2020 LG Electronics, Inc.
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
import WebOSCompositorBase 1.0
import WebOSCompositor 1.0

FocusableView {
    id: root
    layerNumber: 11
    opacity: 0
    visible: opacity > 0

    property string appId: ""

    onOpened: {
        if (root.access) {
            root.opacity = 1;
            timer.restart();
            root.requestFocus();
        } else {
            root.closeView();
            console.warn("AccessControl: Spinner is restricted by the access control policy.");
        }
    }

    onClosed: {
        root.opacity = 0;
        timer.stop();
        root.releaseFocus();
    }

    Timer {
        id: timer
        interval: Settings.local.spinner.timeout
        onTriggered: root.closeView();
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.visible
        hoverEnabled: enabled
        acceptedButtons: Qt.AllButtons

        Rectangle {
            color: "black"
            opacity: Settings.local.spinner.scrimOpacity
            anchors.fill: parent
        }

        AnimatedImage {
            anchors.centerIn: parent
            source: Settings.local.imageResources.spinner
            playing: root.visible
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: Settings.local.spinner.fadeAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    function start(appId) {
        root.appId = appId;
        root.openView();
    }
}
