// Copyright (c) 2016-2021 LG Electronics, Inc.
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
import WebOSServices 1.0
import WebOSCompositorBase 1.0

Item {
    id: root

    property bool connected: false

    property bool acceptAlerts: true
    property bool acceptToasts: true
    property bool acceptPincodePrompts: true
    property string sessionId: ""

    signal restartToastTimer()

    function closePincodePrompt(params) {
        LS.adhoc.call("luna://com.webos.notification", "/closePincodePrompt", JSON.stringify(params));
    }

    property NotificationModel toastModel: NotificationModel {
        id: toastModel
        accept: root.acceptToasts
        modelData: notificationManagerService.toastList !== "" ? JSON.parse(notificationManagerService.toastList) : ({ })

        function parseModelData() {
            if (!modelData || Object.keys(modelData).length === 0)
                 return;
            for (var i = 0; i < count; i++) {
                var d = get(i);
                if (modelData.sourceId === d.sourceId && modelData.message === d.message && modelData.type === d.type) {
                    if (i === count - 1) {
                        set(i, modelData);
                        root.restartToastTimer();
                    } else {
                        append(modelData);
                        remove(i);
                    }
                    return;
                }
            }
            append(modelData);
        }

        function unacceptable() {
            console.warn("AccessControl: Toast is restricted by the access control policy.");
        }
    }

    property NotificationModel alertModel: NotificationModel {
        id: alertModel
        accept: root.acceptAlerts
        modelData: notificationManagerService.alertList !== "" ? JSON.parse(notificationManagerService.alertList) : ({ })

        function parseModelData() {
            if (!modelData || Object.keys(modelData).length === 0)
                 return;
            var i = 0;
            switch (modelData.alertAction) {
            case "closeAll":
                clear();
                return;
            case "close":
                for (i = 0; i < count; i++) {
                    var obj = get(i);
                    if (obj.timestamp === modelData.alertInfo.timestamp) {
                        remove(i);
                        break;
                    }
                }
                return;
            default:
                break;
            }

            // Check if it duplicates
            for (i = 0; i < count; i++) {
                var d = get(i);
                if (modelData.alertInfo.message === d.alertInfo.message &&
                    modelData.alertInfo.sourceId === d.alertInfo.sourceId &&
                    modelData.alertInfo.title === d.alertInfo.title &&
                    modelData.alertInfo.modal === d.alertInfo.modal) {
                        console.warn("Remove duplicated alert - " + JSON.stringify(modelData));
                        remove(i);
                        break;
                }
            }

            // If alert is modal, then place it at top of queue, else at bottom.
            if (count && modelData.alertInfo.modal) {
                for (i = 0; i < count; i++) {
                    if (!get(i).alertInfo.modal) {
                        insert(i, modelData);
                        break;
                    }
                }
                if (i === count)
                    append(modelData);
            } else {
                append(modelData);
            }
        }

        function unacceptable() {
            console.warn("AccessControl: Alert is restricted by the access control policy.");
            if (!modelData || Object.keys(modelData).length === 0)
                 return;
            var failAction = modelData.alertInfo.onFailAction
            if (failAction && Object.keys(failAction).length > 0)
                LS.adhoc.call(failAction.serviceURI, failAction.serviceMethod, JSON.stringify(failAction.launchParams));
        }
    }

    property NotificationModel pincodePromptModel: NotificationModel {
        id: pincodePromptModel
        accept: root.acceptPincodePrompts
        modelData: notificationManagerService.pincodePromptList !== "" ? JSON.parse(notificationManagerService.pincodePromptList) : ({ })

        onModelDataChanged: {
            if (!accept) {
                switch (modelData.pincodePromptAction) {
                case "open":
                    root.closePincodePrompt({"closeType": "close"});
                    break;
                case "close":
                    for (var i = 0; i < count; i++) {
                        var obj = get(i);
                        if (obj.timestamp === modelData.pincodePromptInfo.timestamp) {
                            remove(i);
                            break;
                        }
                    }
                    break;
                default:
                    break;
                }
            }
        }

        function parseModelData() {
            if (!modelData || Object.keys(modelData).length === 0)
                 return;
            if (modelData.pincodePromptAction === "close") {
                for (var i = 0; i < count; i++) {
                    var obj = get(i);
                    if (obj.timestamp === modelData.pincodePromptInfo.timestamp) {
                        remove(i);
                        break;
                    }
                }
            } else {
                append(modelData);
            }
        }

        function unacceptable() {
            console.warn("AccessControl: PincodePrompt is restricted by the access control policy.");
        }
    }

    property Service notificationManagerStatusListener: Service {
        id: notificationManagerStatusListener
        appId: LS.appId
        service: "luna://com.webos.service.bus"
        method: "signal/registerServerStatus"
        sessionId: root.sessionId

        Component.onCompleted: {
            callService({ serviceName: "com.webos.notification", subscribe: true });
        }

        onResponse: (method, payload, token) => {
            var response = JSON.parse(payload);
            root.connected = response.connected;

            if (root.connected)
                notificationManagerService.subscribe();
            else
                notificationManagerService.unsubscribe();
        }

        onSessionIdChanged: {
            cancel();
        }
    }

    property Service notificationManager: Service {
        id: notificationManagerService
        appId: LS.appId
        service: "luna://com.webos.notification"
        sessionId: root.sessionId

        property string toastList: ""
        property string alertList: ""
        property string pincodePromptList: ""

        property int toastToken: 0
        property int alertToken: 0
        property int pincodePromptToken: 0

        function subscribe() {
            toastToken = call(service, "/getToastNotification", '{"subscribe": true}');
            alertToken = call(service, "/getAlertNotification", '{"subscribe": true}');
            pincodePromptToken = call(service, "/getPincodePromptNotification", '{"subscribe": true}');
        }

        function unsubscribe() {
            if (toastToken > 0) {
                cancel(toastToken);
                toastToken = 0;
            }
            if (alertToken > 0) {
                cancel(alertToken);
                alertToken = 0;
            }
            if (pincodePromptToken > 0) {
                cancel(pincodePromptToken);
                pincodePromptToken = 0;
            }
        }

        onResponse: (method, payload, token) => {
            var receivedObj = JSON.parse(payload);

            if (!receivedObj)
                return;
            if (receivedObj.subscribed === true)
                return;
            if (receivedObj.returnValue === false)
                return;

            switch (token) {
            case toastToken:
                if (toastList === payload)
                    return;
                toastList = payload;
                break;
            case alertToken:
                if (alertList === payload)
                    return;
                alertList = payload;
                break;
            case pincodePromptToken:
                if (pincodePromptList === payload)
                    return;
                pincodePromptList = payload;
                break;
            default:
                break;
            }
        }

        onSessionIdChanged: {
            cancel();
        }
    }
}
