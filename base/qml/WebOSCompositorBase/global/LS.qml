// Copyright (c) 2017-2022 LG Electronics, Inc.
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

pragma Singleton

import QtQuick 2.4
import WebOSServices 1.0

QtObject {
    id: root
    objectName: "LS"

    // NOTE: Define properties readonly as they're shared globally

    readonly property string appId: "com.webos.surfacemanager"

    readonly property Service sessionManager: Service {
        readonly property string serviceName: "com.webos.service.account"
        property string sessionId
        property var sessionList

        appId: root.appId
        service: serviceName
        method: "getSessions"

        // Internal properties
        property bool connected: false
        property int serverStatusToken: 0
        property int subscriptionToken: 0

        Component.onCompleted: {
            console.log("LS.sessionManager: checking status for", serviceName);
            serverStatusToken = registerServerStatus(serviceName);
        }

        onConnectedChanged: {
            console.log("LS.sessionManager: connected", connected);
            if (connected) {
                // All valid tokens are positive and 0 means LSMESSAGE_TOKEN_INVALID
                if (subscriptionToken > 0)
                    cancel(subscriptionToken);
                var payload = {subscribe: true};
                subscriptionToken = callService(payload);
            } else {
                cancel(serverStatusToken);
                serverStatusToken = registerServerStatus(serviceName);
            }
        }

        onResponse: (method, payload, token) => {
            console.log("LS.sessionManager: response:", payload, "method:", method, "token:", token);
            var response = JSON.parse(payload);
            switch (token) {
            case serverStatusToken:
                connected = response.connected;
                break;
            case subscriptionToken:
                if (response.returnValue && response.sessions) {
                    sessionList = response.sessions;
                    if (sessionList && sessionList.length > 0) {
                        for (var i = 0; i < sessionList.length; i++) {
                            if (sessionList[i].deviceSetInfo.displayId == compositorWindow.displayId) {
                                sessionId = sessionList[i].sessionId;
                                console.info("LS.sessionManager: sessionId for this display:", sessionId, "displayId:", compositorWindow.displayId);
                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    readonly property Service adhoc: Service {
        appId: root.appId
    }

    readonly property ApplicationManagerService applicationManager: ApplicationManagerService {
        appId: root.appId
        sessionId: root.sessionManager.sessionId

        property var appInfoList: ({})

        onConnectedChanged: {
            console.log("LS.applicationManager: connected", connected);
            if (connected) {
                subscribeAppLifeEvents();
                subscribeApplicationList();
            }
        }

        onJsonApplicationListChanged: {
            var res = jsonApplicationList;
            if (res.apps) {
                var newList = [];
                // Make a hash table by appId
                for (var i = 0; i < res.apps.length; i++) {
                    if (res.apps[i].id)
                        newList[res.apps[i].id] = res.apps[i];
                }
                appInfoList = newList;
            } else if (res.app && res.change !== undefined) {
                // "app" is returned for any change of an individual app
                var newList = appInfoList;
                switch (res.change) {
                    case "added":
                    case "updated":
                        newList[res.app.id] = res.app;
                        break;
                    case "removed":
                        newList[res.app.id] = undefined;
                        break;
                    default:
                        console.warn("Unhandled response for listApps:", applicationList);
                        return;
                }
                appInfoList = newList;
            }
        }
    }

    Component.onCompleted: console.info("Constructed a singleton type:", root);
}
