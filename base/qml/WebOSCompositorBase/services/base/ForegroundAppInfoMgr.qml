// Copyright (c) 2017-2019 LG Electronics, Inc.
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

QtObject {
    id: root

    // Changes will be notified only if enabled is set
    property bool enabled: true
    property var foregroundItems: []

    property bool __init: false
    property var __foregroundItemsNotified: []

    signal foregroundAppInfoChanged

    function checkAndNotify() {
        var diff = false;
        if (foregroundItems.length != __foregroundItemsNotified.length) {
            diff = true;
        } else {
            for (var i = 0; i < foregroundItems.length; i++) {
                if (foregroundItems[i] != __foregroundItemsNotified[i]) {
                    diff = true;
                    break;
                }
            }
        }

        if (diff) {
            if (enabled)
                root.foregroundAppInfoChanged();
            else
                console.warning("Foreground app info changed but skipped notifying it");

            // Update the notified list
            __foregroundItemsNotified = [];
            for (var i = 0; i < foregroundItems.length; i++)
                __foregroundItemsNotified.push(foregroundItems[i]);
        } else {
            console.log("Skipped notifying as the foreground item list is identical");
        }
    }

    onForegroundItemsChanged: {
        if (!__init)
            __init = initialized();
        if (__init)
            checkAndNotify();
    }

    function initialized() {
        for (var i = 0; i < foregroundItems.length; i++)
            if (foregroundItems[i]) return true;
        return false;
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
                windowId: "",
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
