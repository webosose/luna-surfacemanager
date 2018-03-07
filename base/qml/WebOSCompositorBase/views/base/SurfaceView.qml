// Copyright (c) 2017-2018 LG Electronics, Inc.
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
import WebOSCoreCompositor 1.0
import WebOSCompositorBase 1.0

import "../../../WebOSCompositor"

FocusableView {
    id: root

    property WindowModel model
    property bool fill: false
    property bool positioning: false
    property bool consumeMouseEvents: false
    property Item currentItem

    signal surfaceTransformUpdated(Item item)
    signal surfaceAdded(Item item)
    signal surfaceRemoved(Item item)

    Connections {
        target: model

        onSurfaceAdded: {
            if (root.fill) {
                item.scale = Qt.binding(
                        function() {
                            return item.rotation % 180 ? Math.min(root.width / item.height, root.height / item.width) : Math.min(root.width / item.width, root.height / item.height); });
            }

            if (root.positioning) {
                // Align to center
                item.x = Qt.binding(function() { return Utils.center(root.width, item.width); });
                item.y = Qt.binding(function() { return Utils.center(root.height, item.height); });
            }

            root.surfaceTransformUpdated(item);
            root.surfaceAdded(item);
            root.contentChanged();
        }

        onSurfaceRemoved: {
            root.surfaceRemoved(item);
            root.contentChanged();
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.visible && root.consumeMouseEvents
        hoverEnabled: enabled
        visible: enabled
        acceptedButtons: Qt.AllButtons
        onWheel: {}
    }

    onActiveFocusChanged: {
        if (activeFocus && root.currentItem)
            root.currentItem.takeFocus();
    }
}
