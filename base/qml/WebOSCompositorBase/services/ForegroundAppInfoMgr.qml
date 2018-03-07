// Copyright (c) 2017-2018 LG Electronics, Inc.
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

    property var foregroundAppItem: []
    property var service
    property bool init: false

    onForegroundAppItemChanged: {
        if (!init) init = initialized();
        if (init) sendForegroundAppInfo();
    }

    function initialized() {
        for (var i = 0; i < foregroundAppItem.length; i++)
            if (foregroundAppItem[i]) return true;
        return false;
    }

    function getForegroundAppInfoParam(foregroundAppItem) {
        var param, jsonParams;

        console.log("Push foreground app: " + foregroundAppItem.appId);

        if (foregroundAppItem.params) {
            try {
                jsonParams = JSON.parse(foregroundAppItem.params);
            } catch (e) {
                jsonParams = e.message;
            }
        } else jsonParams = {};

        if (foregroundAppItem.isSurfaceGroupRoot() || foregroundAppItem.isPartOfGroup()) {
            param = {
                appId: foregroundAppItem.appId,
                windowId: "",
                processId: foregroundAppItem.processId,
                windowType: foregroundAppItem.type,
                windowGroup: true,
                windowGroupOwner: foregroundAppItem.isSurfaceGroupRoot(),
                windowGroupOwnerId: (foregroundAppItem.isPartOfGroup()) ? foregroundAppItem.surfaceGroup.rootItem.appId : "",
                params: jsonParams
            };
        } else {
            param = {
                appId: foregroundAppItem.appId,
                windowId: "",
                processId: foregroundAppItem.processId,
                windowType: foregroundAppItem.type,
                windowGroup: false,
                params: jsonParams
            };
        }

        return param;
    }

    function getForegroundAppInfo() {
        var jsonParams;
        var params = [];

        for (var i = 0; i < foregroundAppItem.length; i++) {
            if (foregroundAppItem[i] !== null && foregroundAppItem[i] !== undefined) {
                if (Array.isArray(foregroundAppItem[i])) {
                    var items = foregroundAppItem[i];
                    for (var j = 0; j < items.length; j++)
                        params.push(getForegroundAppInfoParam(items[j]));
                } else {
                    params.push(getForegroundAppInfoParam(foregroundAppItem[i]));
                }
            }
        }

        return params;
    }

    function sendForegroundAppInfo() {
        // Push foreground app info to subscribers
        if (service)
            service.pushSubscription("getForegroundAppInfo");
    }
}
