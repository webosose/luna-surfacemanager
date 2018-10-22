// Copyright (c) 2018 LG Electronics, Inc.
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

import "../../WebOSCompositor"

BaseView {
    id: root

    property WindowModel model
    property Item currentItem: null
    property bool allowed: root.access
    property bool reopen: false

    x: compositor.inputMethod.panelRect.x
    y: compositor.inputMethod.panelRect.y
    width: compositor.inputMethod.panelRect.width
    height: compositor.inputMethod.panelRect.height
    clip: compositor.inputMethod.hasPreferredPanelRect

    Binding {
        target: compositor.inputMethod
        property: "panelRect"
        value: if (compositor.inputMethod.hasPreferredPanelRect) {
            // Floating
            compositor.inputMethod.preferredPanelRect
        } else if (compositor.inputMethod.panelSurfaceSize.height > 0) {
            // Anchored at bottom, depending on panel surface size
            Qt.rect(
                0,
                root.parent.height - compositor.inputMethod.panelSurfaceSize.height,
                root.parent.width,
                compositor.inputMethod.panelSurfaceSize.height
            )
        } else {
            // Default
            Qt.rect(
                0,
                root.parent.height - Settings.local.keyboardView.height,
                root.parent.width,
                Settings.local.keyboardView.height
            )
        }
    }

    Connections {
        target: compositor.inputMethod
        onPanelRectChanged: {
            if (!compositor.inputMethod.hasPreferredPanelRect)
                root.reopenView();
        }
        onHasPreferredPanelRectChanged: {
            root.reopenView();
        }
    }

    Item {
        id: keyboardArea
        width: root.width
        height: root.height
        anchors.bottom: root.bottom
        anchors.bottomMargin: -height
    }

    Binding {
        target: compositor.inputMethod
        property: "allowed"
        value: root.allowed
    }

    Connections {
        target: root.model

        onSurfaceAdded: {
            console.log("Adding item " + item + " to " + root);
            if (root.access) {
                item.parent = keyboardArea;
                // Scale while preserving aspect ratio
                item.x = Qt.binding(function() { return Utils.center(keyboardArea.width, item.width); });
                item.y = Qt.binding(function() { return Utils.center(keyboardArea.height, item.height); });
                item.scale = Qt.binding(function() { return Math.min(keyboardArea.width / item.width, keyboardArea.height / item.height); });
                item.visible = true;
                item.opacity = 0.999;
                item.useTextureAlpha = true;
                currentItem = item;
                root.openView();
                root.contentChanged();
                console.log("Item added in " + root + ", currentItem: " + currentItem);
            } else {
                item.close();
                compositor.inputMethod.deactivate();
                console.warn("AccessControl: KeyboardView is restricted by the access control policy.");
            }
        }

        onSurfaceRemoved: {
            console.log("Removing item " + item + " from " + root);
            if (currentItem == item) {
                for (var i = root.model.count - 1; i>= 0; i--) {
                    if (root.model.get(i) != item) {
                        currentItem = root.model.get(i);
                        break;
                    }
                }
                if (currentItem == item) {
                    root.closeView();
                    currentItem = null;
                }
            }
            root.contentChanged();
            console.log("Item removed from " + root + ", currentItem: " + currentItem);
        }
    }

    openAnimation: SequentialAnimation {
        PropertyAnimation {
            target: keyboardArea
            property: "anchors.bottomMargin"
            to: 0
            duration: Settings.local.keyboardView.openAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    closeAnimation: SequentialAnimation {
        PropertyAnimation {
            target: keyboardArea
            property: "anchors.bottomMargin"
            to: -keyboardArea.height
            duration: 0
            easing.type: Easing.InOutCubic
        }
        PauseAnimation {
            // Pause in the middle of re-opening to avoid keyboards in different sizes shown momentarily
            duration: root.reopen ? 500 : 0
        }
    }

    onClosed: {
        if (root.reopen) {
            root.reopen = false;
            if (compositor.inputMethod.active)
                root.openView();
            else
                console.warn("Abort re-opening as the input method appears to be inactive.")
            return;
        }
        if (root.model.count > 0) {
            // Deactivate the input method context as it must be the case
            // that someone else wants to close this view.
            compositor.inputMethod.deactivate();
        }
    }

    function reopenView() {
        if (isOpen) {
            console.log("Re-opening view:", root);
            root.reopen = true;
            root.closeView();
        }
    }
}
