// Copyright (c) 2013-2021 LG Electronics, Inc.
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

FocusScope {
    id: root

    property string rootObjectName: "popupRootElement"
    property bool modal: false
    property bool scrimEnabled: true

    signal scrimClicked
    signal scrimEntered

    signal opened
    signal closed

    Rectangle {
        id: scrim
        color: "black"
        opacity: root.modal ? Settings.local.notification.scrimOpacity : 0
        anchors.fill: parent
        visible: privateObject.isOpen && scrimEnabled

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            enabled: scrimEnabled
            acceptedButtons: Qt.AllButtons
            onClicked: (mouse) => { scrimClicked(); }
            onEntered: {}
            onDoubleClicked: (mouse) => {}
            onWheel: (wheel) => {}
            onPressed: (mouse) => {}
            onPressAndHold: (mouse) => {}
            hoverEnabled: true
        }
    }

    visible: privateObject.isOpen

    function findRootItem(objectName) {
        var next = root;

        while (next && next.parent) {
            next = next.parent;
            if (objectName === next.objectName)
                break;
        }
        return next;
    }

    function open() {
        if (privateObject.isOpen)
            return;
        if (scrimEnabled) {
            privateObject.originalParent = parent;
            parent = findRootItem(rootObjectName);
            scrim.parent = parent;
        }
        privateObject.isOpen = true;
        root.opened();
    }

    function close() {
        if (!privateObject.isOpen)
            return;
        if (scrimEnabled) {
            parent = privateObject.originalParent;
            scrim.parent = root;
        }
        privateObject.isOpen = false;
        root.closed();
    }

    QtObject {
        id: privateObject

        property QtObject originalParent: null
        property bool isOpen: false
    }
}
