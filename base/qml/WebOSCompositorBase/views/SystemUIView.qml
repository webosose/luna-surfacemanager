// Copyright (c) 2019-2021 LG Electronics, Inc.
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

// Use the same handler with PopopView for now
import "base/popuphandler.js" as PopupHandler

SurfaceView {
    id: root
    layerNumber: 10
    fill: false
    positioning: false
    consumeMouseEvents: false

    property bool consumeKeyEvents: false
    property url containerQml: Qt.resolvedUrl("./base/SystemUISurface.qml")

    property Component component: Qt.createComponent(root.containerQml, root)

    function addonFilter(addon) {
        console.log("SystemUIView: addonFilter for " + addon);
        const match = (pat) => addon.startsWith(pat);
        if (addon && !Settings.local.addon.directories.some(match)) {
            console.warn("SystemUIView: addon denied by path", addon);
            return false;
        }
        return true;
    }

    onSurfaceAdded: (item) => {
        if (root.access) {
            if (PopupHandler.addPopup(component, root, item)) {
                item.addonFilter = root.addonFilter;
                currentItem = item;
                if (root.consumeKeyEvents) {
                    PopupHandler.giveFocus();
                    root.requestFocus();
                }
                root.openView();
            } else {
                console.warn("SystemUIView: Error creating object");
            }
        } else {
            item.close();
            console.warn("AccessControl: " + root + " is restricted by the access control policy.");
        }
    }

    onSurfaceRemoved: (item) => {
        // Remove and close the surface item.
        var popupCount = PopupHandler.removePopup(item);
        console.log("SystemUIView: releasing", popupCount);
        item.addonFilter = undefined;
        if (popupCount > 0) {
            currentItem = PopupHandler.getCurrentPopup();
            if (root.consumeKeyEvents) {
                console.log("SystemUIView: an item was removed, focus is moved to the previous item");
                PopupHandler.giveFocus();
            }
        } else {
            currentItem = null;
            if (root.consumeKeyEvents)
                root.releaseFocus();
            root.closeView();
        }
    }

    onClosed: {
        PopupHandler.closeAllPopup();
    }

    onFocusChanged: {
        if (root.consumeKeyEvents) {
            console.log("SystemUIView: focus", focus);
            if (focus)
                PopupHandler.giveFocus();
        }
    }
}
