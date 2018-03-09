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

    width: currentItem ? currentItem.width : 0
    height: currentItem ? currentItem.height : 0
    anchors.bottom: root.parent.bottom
    anchors.bottomMargin: -root.height

    property WindowModel model
    property Item currentItem: null
    property bool allowed: root.access

    Binding {
        target: compositor
        property: "inputMethod.allowed"
        value: root.allowed
    }

    Connections {
        target: root.model

        onSurfaceAdded: {
            console.log("Adding item " + item + " to " + root);
            if (root.access) {
                item.parent = root;
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
            target: root
            property: "anchors.bottomMargin"
            to: 0
            duration: Settings.local.keyboardView.openAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    closeAnimation: SequentialAnimation {
        PropertyAnimation {
            target: root
            property: "anchors.bottomMargin"
            to: -root.height
            duration: Settings.local.keyboardView.openAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    onClosed: {
        if (root.model.count > 0) {
            // Deactivate the input method context as it must be the case
            // that someone else wants to close this view.
            compositor.inputMethod.deactivate();
        }
    }
}
