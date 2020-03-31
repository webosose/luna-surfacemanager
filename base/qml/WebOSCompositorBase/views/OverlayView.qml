// Copyright (c) 2013-2020 LG Electronics, Inc.
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

SurfaceView {
    id: root
    layerNumber: 3
    fill: true
    positioning: true
    consumeMouseEvents: true

    onSurfaceAdded: {
        if (root.access) {
            // Close currentItem if exists
            if (currentItem !== null)
                currentItem.close();
            currentItem = item;
            currentItem.parent = root;
            currentItem.useTextureAlpha = true;
            root.requestFocus();
            root.openView();
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
        item.close();
    }

    onClosed: {
        if (currentItem)
            currentItem.close();
    }
}
