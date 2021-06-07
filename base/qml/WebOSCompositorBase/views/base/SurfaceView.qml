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
import WebOSCoreCompositor 1.0
import WebOSCompositorBase 1.0
import WebOSCompositor 1.0

FocusableView {
    id: root
    hasForegroundItem: true

    property WindowModel model
    property bool fill: false
    property bool positioning: false
    property bool consumeMouseEvents: false
    property bool focusOnSurface: true
    property Item currentItem
    property Item newItem
    property bool keepLastFrame: false
    property Item itemToBeHidden

    // For multi-item view
    property bool hasMultiItems: false
    property var items: []

    property SequentialAnimation swapAnimation

    signal surfaceTransformUpdated(Item item)
    signal surfaceAdded(Item item)
    signal surfaceRemoved(Item item)

    // For surface group
    property bool grouped: false
    property var groupedItems: []
    signal groupFocused

    QtObject {
        id: surfaceHighlight

        property Component component: Qt.createComponent("../debug/SurfaceHighlight.qml", root)

        function enable(item) {
            component.createObject(item, {"objectName": item.objectName + "_surfaceHighlight", "sourceItem": item, "sourceView": root});
        }

        function disable(item) {
            for (var i = 0; i < item.children.length; i++)
                if (item.children[i].objectName === item.objectName + "_surfaceHighlight")
                    item.children[i].destroy();
        }
    }

    Connections {
        target: model

        function onSurfaceAdded(item) {
            console.log("Adding item " + item + " to " + root);

            // Fast-forward any on-going animations
            if (root.openAnimation && root.openAnimation.running) {
                console.warn("Fast-forward on-going openAnimation in " + root + " to proceed another with " + item);
                root.openAnimation.complete();
            }
            if (root.swapAnimation && root.swapAnimation.running) {
                console.warn("Fast-forward on-going swapAnimation in " + root + " to proceed another with " + item);
                root.swapAnimation.complete();
            }

            // Initial scale and positioning
            if (root.fill)
                item.scale = item.rotation % 180 ? Math.min(root.width / item.height, root.height / item.width) : Math.min(root.width / item.width, root.height / item.height);
            if (root.positioning) {
                item.x = Utils.center(root.width, item.width);
                item.y = Utils.center(root.height, item.height);
            }

            root.newItem = item;
            root.newItem.parent = root;

            if (root.currentItem && root.swapAnimation) {
                console.log("Triggering swapAnimation " + root.swapAnimation + " for " + root + " with " + currentItem + " to " + newItem);
                root.swapAnimation.alwaysRunToEnd = true;
                root.swapAnimation.start();
            } else {
                console.log("Skipping swapAnimation for " + root);
                root.handleSurfaceAdded();
            }
            Utils.pmLog.info("NL_VSC", {"app_id": item.appId, "visible":true, "window_type":item.type});
        }

        function onSurfaceRemoved(item) {
            console.log("Removing item " + item + " from " + root);
            Utils.pmLog.info("NL_VSC", {"app_id": item.appId, "visible":false, "window_type":item.type});
            root.surfaceRemoved(item);
            root.contentChanged();
            console.log("Item removed from " + root + ", currentItem: " + currentItem);

            surfaceHighlight.disable(item);
        }
    }

    Connections {
        target: root.swapAnimation
        function onStopped() {
            console.log("Finished swapAnimation finished in " + root + ", currentItem: " + root.currentItem + ", newItem: " + root.newItem);
            root.handleSurfaceAdded();
        }
    }

    function handleSurfaceAdded() {
        // Finalize scale and positioning
        var item = root.newItem;
        if (root.fill) {
            item.scale = Qt.binding(
                function() {
                    return item.rotation % 180 ? Math.min(root.width / item.height, root.height / item.width) : Math.min(root.width / item.width, root.height / item.height);
            });
        }
        if (root.positioning) {
            // Align to center
            item.x = Qt.binding(function() { return Utils.center(root.width, item.width); });
            item.y = Qt.binding(function() { return Utils.center(root.height, item.height); });
        }
        root.surfaceTransformUpdated(root.newItem);
        root.surfaceAdded(root.newItem);
        Utils.performanceLog.timeEnd("APP_LAUNCH", {"APP_ID": root.newItem.appId});
        root.contentChanged();

        // currentItem is supposed to be set by the inheritor.
        // Warn in case it isn't.
        if (root.currentItem == root.newItem) {
            console.log("Item added in " + root + ", currentItem: " + root.currentItem);
            surfaceHighlight.enable(root.currentItem);
        } else {
            console.warn("FIXME: currentItem for " + root + " seems not set correctly, currentItem: " + root.currentItem, ", newItem: " + root.newItem);
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.visible && root.consumeMouseEvents
        hoverEnabled: enabled
        visible: enabled
        acceptedButtons: Qt.AllButtons
        onWheel: (wheel) => {}
    }

    onFocused: {
        if (grouped) {
            root.groupFocused();
        } else if (activeFocus && root.currentItem) {
            if (root.focusOnSurface)
                root.currentItem.takeFocus();
            else
                console.warn("focusOnSurface is unset, skip setting focus on the current surface");
        }
    }

    Connections {
        target: root.currentItem
        function onItemAboutToBeHidden() {
            console.log("Item " + currentItem + " in " + root + " is about to be hidden");
            if (root.currentItem && root.keepLastFrame) {
                console.info("Keep last frame of item " + currentItem + " until " + root + " is closed completely");
                root.currentItem.grabLastFrame();
                root.itemToBeHidden = root.currentItem;
                // Trigger closeAnimation
                root.closeView();
            }
        }
    }

    Connections {
        target: compositor
        function onSurfaceMapped(item) {
            if (root.itemToBeHidden && root.itemToBeHidden == item) {
                console.info("Abort closing item " + root.itemToBeHidden + " from " + root);
                root.itemToBeHidden.releaseLastFrame();
                root.itemToBeHidden = null;
                root.openView();
            }
        }
    }

    onClosed: {
        console.log("SurfaceView " + root + " is closed");
        if (root.itemToBeHidden) {
            console.info("Release last frame of item " + currentItem + " from " + root);
            root.itemToBeHidden.releaseLastFrame();
            root.itemToBeHidden = null;
        }
    }
}
