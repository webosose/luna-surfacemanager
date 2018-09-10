// Copyright (c) 2013-2019 LG Electronics, Inc.
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

import "qrc:/WebOSCompositor"
import "file:///usr/lib/qml/WebOSCompositor"

SurfaceView {
    id: root
    layerNumber: 1
    fill: true
    positioning: true
    consumeMouseEvents: true

    onSurfaceAdded: {
        if (root.access) {
            var oldItem = currentItem || null;
            currentItem = item;
            currentItem.parent = root;
            currentItem.useTextureAlpha = true;
            root.requestFocus();
            root.openView();
            if (oldItem)
                oldItem.fullscreen = false;
        } else {
            item.close();
            console.warn("AccessControl: " + root + " is restricted by the access control policy.");
        }
    }

    onSurfaceRemoved: {
        if (currentItem == item)
            currentItem = null;
        if (!currentItem) {
            root.releaseFocus();
            root.closeView();
        }
        item.parent = null;
    }

    Connections {
        target: compositor

        onSurfaceMapped: {
            console.log("item:", item);
            checkFullscreen(item);
        }

        onSurfaceUnmapped: {
            console.log("item:", item);
            resetFullscreen(item);
        }

        onSurfaceDestroyed: {
            console.log("item:", item);
            resetFullscreen(item);
        }

        onFullscreenRequested: {
            console.log("item:", item);
            if (!checkFullscreen(item))
                console.warn("request ignored, item:", item);
        }

        function checkFullscreen(item) {
            if (!item.isProxy() && !item.isPartOfGroup() &&
                (item.type == "_WEBOS_WINDOW_TYPE_CARD" || item.type == "_WEBOS_WINDOW_TYPE_RESTRICTED")) {
                console.log("item: " + item + ", currently " + currentItem);

                // Fullscreen surface item transition sequence
                //  1) State change for currentItem to minimized
                //  2) State change for new item to fullscreen
                //  3) Turn on fullscreen flag for new item as true (add to the model)
                //  4) Set new item as currentItem
                //  5) Turn off fullscreen flag for old currentItem

                if (root.currentItem)
                    root.currentItem.state = Qt.WindowMinimized;

                // Add item to the window model
                item.state = Qt.WindowFullScreen;
                item.fullscreen = true;

                return true;
            }

            return false;
        }

        function resetFullscreen(item) {
            if (item == root.currentItem)
                item.fullscreen = false;
        }
    }
}
