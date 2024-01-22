// Copyright (c) 2017-2020 LG Electronics, Inc.
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

QtObject {
    id: root

    // Changes will be notified only if enabled is set
    property bool enabled: true
    property var foregroundItems: []

    property bool __init: false
    property var __foregroundItemsToCheck: []

    signal foregroundAppInfoChanged

    Component.onCompleted: {
        if (compositor.loaded) {
            bindForegroundItems();
        } else {
            console.warn("pending bindForegroundItems until compositor loaded fully");
            compositor.loadCompleted.connect(bindForegroundItems);
        }
    }

    onForegroundItemsChanged: {
        if (!__init)
            __init = initialized();
        if (__init)
            checkAndNotify();
    }

    onEnabledChanged: {
        if (__init && enabled)
            renewForegroundItems();
    }

    function initialized() {
        for (var i = 0; i < foregroundItems.length; i++)
            if (foregroundItems[i]) return true;
        return false;
    }

    function bindForegroundItems() {
        console.info("binding foregroundItems of all compositor windows:", compositor.windows.length);
        for (var i = 0; i < compositor.windows.length; i++)
            compositor.windows[i].viewsRoot.foregroundItemsChanged.connect(updateForegroundItems);
        // This method should be called only once as otherwise connections may leak
    }

    function updateForegroundItems() {
        console.log("updating foregroundAppInfoMgr.foregroundItems:", compositor.windows.length);
        var mergedList = [];
        for (var i = 0; i < compositor.windows.length; i++) {
            // "length" property of foregroundItems in windows other than
            // the primary appears as undefined. So null-checking is used here
            // for the loop-end condition. It needs to be revisited later
            // although it could be a bug or restriction of QML engine.
            for (var j = 0; compositor.windows[i].viewsRoot.foregroundItems[j]; j++) {
                console.log("foregroundItem, window:", i, "appId:", compositor.windows[i].viewsRoot.foregroundItems[j].appId);
                mergedList.push(compositor.windows[i].viewsRoot.foregroundItems[j]);
            }
        }
        foregroundItems = mergedList;
    }

    function renewForegroundItems() {
        console.log("renewing foregroundAppInfoMgr.foregroundItems using Utils.foregroundList:", compositor.windows.length);
        var mergedList = [];
        for (var i = 0; i < compositor.windows.length; i++)
            mergedList.push(Utils.foregroundList(compositor.windows[i].viewsRoot.children));
        foregroundItems = mergedList;
    }

    function checkAndNotify() {
        var diff = false;
        if (foregroundItems.length != __foregroundItemsToCheck.length) {
            diff = true;
        } else {
            for (var i = 0; i < foregroundItems.length; i++) {
                if (foregroundItems[i].appId !== __foregroundItemsToCheck[i].appId ||
                    foregroundItems[i].displayId != __foregroundItemsToCheck[i].displayId) {
                    diff = true;
                    break;
                }
            }
        }

        if (enabled) {
            if (diff) {
                root.foregroundAppInfoChanged();
                // Update the item list to check difference
                __foregroundItemsToCheck = [];
                for (var i = 0; i < foregroundItems.length; i++) {
                    var ii = {
                        "appId":      foregroundItems[i].appId,
                        "displayId":  foregroundItems[i].displayId
                    }
                    __foregroundItemsToCheck.push(ii);
                }
            } else {
                console.log("Skipped notifying as the foreground item list is identical");
            }
        } else {
            console.log("Skipped notifying as enabled is false");
        }
    }

    function getForegroundAppInfoParam(item) {
        var param, jsonParams;

        if (item.params) {
            try {
                jsonParams = JSON.parse(item.params);
            } catch (e) {
                jsonParams = e.message;
            }
        } else jsonParams = {};

        param = {
                appId: item.appId,
                displayId: item.displayId,
                processId: item.processId,
                windowType: item.type,
                windowGroup: false,
                params: jsonParams
        };

        if (item.isSurfaceGroupRoot() || item.isPartOfGroup()) {
            param['windowGroup'] = true;
            param['windowGroupOwner'] = item.isSurfaceGroupRoot();
            param['windowGroupOwnerId'] = (item.isPartOfGroup()) ? item.surfaceGroup.rootItem.appId : "";
        }

        return param;
    }

    function getForegroundAppInfo() {
        var jsonParams;
        var params = [];

        for (var i = 0; i < foregroundItems.length; i++) {
            if (foregroundItems[i] !== null && foregroundItems[i] !== undefined) {
                if (Array.isArray(foregroundItems[i])) {
                    var items = foregroundItems[i];
                    for (var j = 0; j < items.length; j++)
                        params.push(getForegroundAppInfoParam(items[j]));
                } else {
                    params.push(getForegroundAppInfoParam(foregroundItems[i]));
                }
            }
        }

        return params;
    }
}
