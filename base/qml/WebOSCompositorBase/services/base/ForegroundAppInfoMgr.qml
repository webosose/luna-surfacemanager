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

    property bool init: false
    property var foregroundItems

    signal foregroundAppInfoChanged

    onForegroundItemsChanged: {
        if (!init) init = initialized();
        if (init) root.foregroundAppInfoChanged();
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

        if (item.isSurfaceGroupRoot() || item.isPartOfGroup()) {
            param = {
                appId: item.appId,
                windowId: "",
                processId: item.processId,
                windowType: item.type,
                windowGroup: true,
                windowGroupOwner: item.isSurfaceGroupRoot(),
                windowGroupOwnerId: (item.isPartOfGroup()) ? item.surfaceGroup.rootItem.appId : "",
                params: jsonParams
            };
        } else {
            param = {
                appId: item.appId,
                windowId: "",
                processId: item.processId,
                windowType: item.type,
                windowGroup: false,
                params: jsonParams
            };
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
