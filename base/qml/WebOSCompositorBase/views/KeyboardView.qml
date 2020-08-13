// Copyright (c) 2018-2020 LG Electronics, Inc.
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
import WebOSCompositor 1.0

BaseView {
    id: root

    property WindowModel model
    property Item currentItem: null
    property bool allowed: root.access
    property bool reopenInProgress: false
    property bool needToReopen: false
    property Item itemToBeHidden: null

    property QtObject inputMethod: QtObject {
        // Default values that deliberately prevent the view working
        property bool active: false
        property bool allowed: false
        property rect panelRect: Qt.rect(0, 0, 0, 0)
        property size panelSurfaceSize: Qt.size(0, 0)
        property rect preferredPanelRect: Qt.rect(0, 0, 0, 0)
        property bool hasPreferredPanelRect: false
    }

    x: inputMethod.panelRect.x
    y: inputMethod.panelRect.y
    width: inputMethod.panelRect.width
    height: inputMethod.panelRect.height
    clip: true

    Binding {
        // Lazy binding that takes effect only when the actual WaylandInputMethod gets available
        property InputMethod primaryInputMethod: compositor.inputMethod
        target: root
        property: "inputMethod"
        when: primaryInputMethod.methods[compositorWindow.displayId] || false
        value: primaryInputMethod.methods[compositorWindow.displayId]
    }

    Binding {
        target: inputMethod
        property: "panelRect"
        when: inputMethod.active
        value: if (inputMethod.hasPreferredPanelRect) {
            // Floating
            inputMethod.preferredPanelRect
        } else if (inputMethod.panelSurfaceSize.height > 0) {
            // Anchored at bottom, depending on panel surface size
            Qt.rect(
                0,
                root.parent.height - inputMethod.panelSurfaceSize.height,
                root.parent.width,
                inputMethod.panelSurfaceSize.height
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
        target: inputMethod
        onPanelRectChanged: {
            if (inputMethod.active && !inputMethod.hasPreferredPanelRect && root.needToReopen)
                root.reopenView();
        }
        onHasPreferredPanelRectChanged: {
            if (inputMethod.active)
                root.reopenView();
            else
                root.needToReopen = true;
        }
    }

    Item {
        id: keyboardArea
        width: root.width
        height: root.height
        anchors.bottom: root.bottom
        // No bottomMargin allowed when it's open
        anchors.bottomMargin: root.isOpen ? 0 : -height
    }

    Binding {
        target: inputMethod
        property: "allowed"
        value: root.allowed
    }

    Connections {
        target: root.model

        onSurfaceAdded: {
            console.log("Adding item " + item + " to " + keyboardArea + " of " + root);
            if (root.access) {
                item.parent = keyboardArea;
                // Scale while preserving aspect ratio
                item.x = Qt.binding(function() { return Utils.center(keyboardArea.width, item.width); });
                item.y = Qt.binding(function() { return Utils.center(keyboardArea.height, item.height); });
                // New surface is in the front to prevent surfaces from tearing down
                if (currentItem !== null)
                    currentItem.z = 0;
                item.z = 1;
                item.scale = Qt.binding(function() { return Math.min(keyboardArea.width / item.width, keyboardArea.height / item.height); });
                item.visible = true;
                item.useTextureAlpha = true;
                currentItem = item;
                root.openView();
                root.contentChanged();
                console.log("Item added in " + root + ", currentItem: " + currentItem);
            } else {
                item.close();
                inputMethod.deactivate();
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
            from: -keyboardArea.height
            to: 0
            duration: Settings.local.keyboardView.slideAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    closeAnimation: SequentialAnimation {
        PropertyAnimation {
            target: keyboardArea
            property: "anchors.bottomMargin"
            from: 0
            to: -keyboardArea.height
            duration: root.reopenInProgress ? 0 : Settings.local.keyboardView.slideAnimationDuration
            easing.type: Easing.InOutCubic
        }
        PauseAnimation {
            // Pause in the middle of re-opening to avoid keyboards in different sizes shown momentarily
            duration: root.reopenInProgress ? 500 : 0
        }
    }

    Connections {
        target: root.currentItem
        onItemAboutToBeHidden: {
            console.log("Item " + currentItem + " in " + root + " is about to be hidden");
            if (root.currentItem) {
                console.info("Keep last frame of item " + currentItem + " until " + root + " is closed completely");
                root.currentItem.grabLastFrame();
                root.itemToBeHidden = root.currentItem;
            }
            // Trigger closeAnimation
            root.closeView();
        }
    }

    onClosed: {
        console.log("KeyboardView " + root + " is closed");
        if (root.itemToBeHidden) {
            console.info("Release last frame of item " + currentItem + " from " + root);
            root.itemToBeHidden.releaseLastFrame();
            root.itemToBeHidden = null;
        }

        if (root.reopenInProgress) {
            root.reopenInProgress = false;
            if (inputMethod.active)
                root.openView();
            else
                console.warn("Abort re-opening as the input method appears to be inactive.")
            return;
        }
        if (!root.isOpen && root.model.count > 0) {
            // Deactivate the input method context as it must be the case
            // that someone else wants to close this view.
            inputMethod.deactivate();
        }
    }

    function reopenView() {
        root.needToReopen = false;
        if (root.isOpen) {
            console.log("Re-opening view:", root);
            root.reopenInProgress = true;
            root.closeView();
        }
    }
}
