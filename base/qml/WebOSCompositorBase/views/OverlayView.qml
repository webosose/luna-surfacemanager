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

SurfaceView {
    id: root
    layerNumber: 2
    fill: true
    positioning: true
    consumeMouseEvents: true

    Connections {
        target: root.model

        onSurfaceAdded: {
            if (root.access) {
                console.log("Surface added to OverlayView: " + item);
                if (currentItem !== null)
                    currentItem.close();
                currentItem = item;
                root.requestFocus();
                item.parent = root;
                item.opacity = 0.999;
                item.useTextureAlpha = true;
                root.openView();
            } else {
                item.close();
                console.warn("AccessControl: OverlayView is restricted by the access control policy.");
            }
        }

        onSurfaceRemoved: {
            console.log("Surface removed from OverlayView: " + item);
            if (currentItem == item)
                currentItem = null;
            root.releaseFocus();
            item.close();
            if (!currentItem)
                root.closeView();
        }
    }

    onClosed: {
        if (currentItem)
            currentItem.close();
    }
}
