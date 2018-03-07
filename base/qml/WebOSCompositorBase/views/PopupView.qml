// Copyright (c) 2013-2018 LG Electronics, Inc.
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

import "../../WebOSCompositor"

import "base/popuphandler.js" as PopupHandler

SurfaceView {
    id: root
    layerNumber: 5
    fill: false
    positioning: false
    consumeMouseEvents: false

    property Component component: Qt.createComponent("base/PopupSurface.qml", root)

    Connections {
        target: root.model

        onSurfaceAdded: {
            if (root.access) {
                console.log("PopupView: Surface added", item);
                if (PopupHandler.addPopup(component, root, item)) {
                    currentItem = item;
                    PopupHandler.giveFocus();
                    root.requestFocus();
                    root.openView();
                } else {
                    console.warn("PopupView: Error creating object");
                }
            } else {
                item.close();
                console.warn("AccessControl: PopupView is restricted by the access control policy.");
            }
        }

        onSurfaceRemoved: {
            console.log("PopupView: Surface removed", item);
            // Remove and close the surface item.
            var popupCount = PopupHandler.removePopup(item);
            console.log("PopupView: releasing", popupCount);
            if (popupCount > 0) {
                console.log("PopupView: Popup was removed, focus is moved to the previous item");
                currentItem = PopupHandler.getCurrentPopup();
                PopupHandler.giveFocus();
            } else {
                currentItem = null;
                root.releaseFocus();
                root.closeView();
            }
        }
    }

    onFocusChanged: {
        console.log("PopupView: focus", focus);
        if (focus)
            PopupHandler.giveFocus();
    }

    onClosed: {
        PopupHandler.closeAllPopup();
    }
}
