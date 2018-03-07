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
    y: root.parent.height

    property WindowModel model
    property Item currentItem: null

    Connections {
        target: root.model

        onSurfaceAdded: {
            if (root.access) {
                item.parent = root;
                item.visible = true;
                item.opacity = 0.999;
                item.useTextureAlpha = true;
                currentItem = item;
                root.openView();
                root.contentChanged();
            } else {
                item.close();
                compositor.inputMethod.deactivate();
                console.warn("AccessControl: KeyboardView is restricted by the access control policy.");
            }
        }

        onSurfaceRemoved: {
            var i = root.model.count - 1;
            while (i >= 0) {
                if (root.model.get(i) != item)
                    break;
                i--;
            }
            currentItem = i >= 0 ? root.model.get(i) : null;
            root.contentChanged();
            if (currentItem == null)
                root.closeView();
        }
    }

    openAnimation: SequentialAnimation {
        PropertyAnimation {
            target: root
            property: "y"
            to: root.parent.height - (root.currentItem ? root.currentItem.height : 0)
            duration: Settings.local.keyboardView.openAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    closeAnimation: SequentialAnimation {
        PropertyAnimation {
            target: root
            property: "y"
            to: root.parent.height
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
