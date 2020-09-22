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
import WebOSServices 1.0

SettingsService {
    id: root
    sessionId: LS.sessionManager.sessionId

    property QtObject settings: QtObject {
        property alias cached: root.cached
        property alias bootStatus: root.bootStatus
        property alias screenRotation: root.screenRotation
    }

    property QtObject l10n: QtObject {
        property alias tr: root.emptyString
        property bool isRTL: false
        property string locale: "en-US"
    }

    property var defaultSubscriptions: []
    property var extraSubscriptions: []

    property var __subscriptions: defaultSubscriptions
    property var __statusTokens: []
    property var __pendingRequests: []
    property var __subscribedRequestMap: ({})
    property var __subscribedKeyValueMap: ({})

    function subscribeOnDemand(serviceName, methodName, param, raw, sessionId) {
        if (param["subscribe"] !== true)
            param["subscribe"] = true;

        var sortedParam = JSON.stringify(__sortObject(param));
        var keyGenerated = serviceName + methodName + sortedParam + raw;
        if (sessionId)
            keyGenerated += sessionId;

        if (__subscribedKeyValueMap[keyGenerated] == undefined) {
            // serviceName x methodName should provide only one matching item.
            var targetPrototype = __subscriptions.filter(
                function (element) {
                    return (element.service === serviceName &&
                    element.method === methodName);
                }
            )[0];

            if (targetPrototype) {
                var subscribeItem = Object.create(targetPrototype);

                subscribeItem["params"] = sortedParam;
                subscribeItem["key"] = keyGenerated;
                subscribeItem["raw"] = raw;
                if (sessionId)
                    subscribeItem["sessionId"] = sessionId;

                console.info("Register subscription to " + serviceName + "/" + methodName + " " + sortedParam + " " + sessionId);
                __subscribeService(subscribeItem);
                __subscribedKeyValueMap[keyGenerated] =
                    Qt.createQmlObject("import QtQuick 2.4; QtObject { property var value }", root);
            } else {
                console.warn("Unsupported subscription: " + serviceName + "/" + methodName + " " + sessionId);
                return;
            }
        }

        return __subscribedKeyValueMap[keyGenerated].value;
    }

    function __sortObject(obj) {
        return Object.keys(obj).sort().reduce(
            function (result, key) {
                if (typeof obj[key] === 'object') {
                    if (obj[key] instanceof Array)
                        result[key] = obj[key].sort();
                    else
                        result[key] = __sortObject(obj[key]);
                } else {
                    result[key] = obj[key];
                }
                return result;
            }, {}
        );
    }

    function __subscribeService(subscribeItem) {
        var sessionId = !subscribeItem.useSession ? "no-session" : subscribeItem.sessionId;
        if (subscribeItem.connected != undefined && subscribeItem.connected) {
            subscribeItem.token = call("luna://" + subscribeItem.service,
                                       "/" + subscribeItem.method,
                                       subscribeItem.params,
                                       undefined,
                                       sessionId);

            if (subscribeItem.token == 0) {
                console.warn("Failed to subscribe to", subscribeItem.service);
            } else {
                console.log("Subscribed to " + subscribeItem.service + "/" +
                    subscribeItem.method + " " + subscribeItem.params +
                    ", token: " + subscribeItem.token +
                    ", sessionId: " + subscribeItem.sessionId);
                __subscribedRequestMap[subscribeItem.token] = subscribeItem;
                return true;
            }
        }

        console.log("Subscription pending for " + subscribeItem.service + "/" +
            subscribeItem.method + " " + subscribeItem.params +
            ", token: " + subscribeItem.token +
            ", sessionId: " + subscribeItem.sessionId);

        __pendingRequests.push(subscribeItem);

        return false;
    }

    function __updateServerStatus(serviceName, connected) {
        // Update service status in prototype
        __subscriptions.reduce(
            function (xs, x) {
                if (x.service === serviceName)
                    x.connected = connected;
            }, {}
        );

        if (connected) {
            __pendingRequests.reduceRight(
                function (xs, x, index, object) {
                    if (x.service === serviceName) {
                        x.connected = true;
                        // Remove subscribed item from the pending list.
                        object.splice(index, 1);
                        __subscribeService(x);
                    }
                }, {}
            );
        } else {
            for (var i in __subscribedRequestMap) {
                if (__subscribedRequestMap[i].service === serviceName) {
                    console.warn("Service is disconnected. Pending again for " +
                        __subscribedRequestMap[i].service + "/" + __subscribedRequestMap[i].method +
                        " params: " + __subscribedRequestMap[i].params);

                    if (!("sessionId" in __subscribedRequestMap[i]) ||
                        !(__subscribedRequestMap[i].useSession))
                        __pendingRequests.push(__subscribedRequestMap[i]);
                    else if (__subscribedRequestMap[i].sessionId && __subscribedRequestMap[i].sessionId == root.sessionId)
                        __pendingRequests.push(__subscribedRequestMap[i]);
                    else
                        console.warn("Pending subscription is removed. token: " + i,
                            __subscribedRequestMap[i].service + "/" + __subscribedRequestMap[i].method +
                            " params: " + __subscribedRequestMap[i].params);
                    cancel(i);
                    delete __subscribedRequestMap[i];
                }
            }
        }
    }

    function __listenServerStatus() {
        __statusTokens = [];
        var token;
        for (var item in __subscriptions) {
            // Assume disconnected
            __updateServerStatus(__subscriptions[item].service, false);
            var useSession = __subscriptions[item].useSession ? true : false;
            token = registerServerStatus(__subscriptions[item].service, useSession);
            if (token != 0) {
                console.info("registerServerStatus done for", __subscriptions[item].service, token, sessionId, useSession);
                __statusTokens.push(token);
            } else {
                console.error("Error on registerServerStatus for", __subscriptions[item].service, sessionId, useSession);
            }
        }
    }

    Component.onCompleted: {
        console.log("Start subscribing to SettingsService");
        subscribe();

        // Merge subscription lists
        if (extraSubscriptions.length > 0) {
            console.log("Merge subscription lists: " + __subscriptions.length + " + " + extraSubscriptions.length);
            __subscriptions = __subscriptions.concat(extraSubscriptions);
        }

        __listenServerStatus();
    }

    onL10nLoadSucceeded:    console.info("Localization: Loaded", file);
    onL10nInstallSucceeded: console.info("Localization: Installed", file);
    onL10nLoadFailed:       console.warn("Localization: Failed to load", file);
    onL10nInstallFailed:    console.warn("Localization: Failed to install", file);
    onError:                console.warn("Localization: An error occurred,", errorText);

    onCurrentLocaleChanged: {
        if (currentLocale && currentLocale !== "")
            l10n.locale = currentLocale;
        if (currentLocale &&
            (currentLocale.indexOf("he-") > -1 || currentLocale.indexOf("ar-") > -1 ||
            currentLocale.indexOf("ur-") > -1 || currentLocale.indexOf("ku-") > -1 ||
            currentLocale.indexOf("fa-") > -1))
            l10n.isRTL = true;
        else
            l10n.isRTL = false;
        console.info("Localization: UI Locale changed. locale: " + l10n.locale + "(" + currentLocale + "), isRTL: " + l10n.isRTL);
    }

    onResponse: {
        if (__statusTokens.indexOf(token) != -1) {
            // Response for registerServerStatus
            var response = JSON.parse(payload);
            if (response.connected) {
                console.info("Connected to", response.serviceName);
                __updateServerStatus(response.serviceName, true);
            } else {
                console.warn("Disconnected from", response.serviceName);
                __updateServerStatus(response.serviceName, false);
            }
        } else {
            // Response for subscriptions
            var item = __subscribedRequestMap[token];

            if (item) {
                var response = JSON.parse(payload);
                if (item.raw) {
                    __subscribedKeyValueMap[item.key].value = response;
                    console.log("Value updated(raw mode): " + item.key + "=" + __subscribedKeyValueMap[item.key].value);
                } else {
                    if (response.returnValue !== undefined && response.returnValue) {
                        var handler_return = item.handler(response);
                        if (handler_return != undefined) {
                            __subscribedKeyValueMap[item.key].value = handler_return;
                            console.log("Value updated: " + item.key + "=" + __subscribedKeyValueMap[item.key].value);
                        } else {
                            console.warn("Unhandled response for", item.service, payload);
                        }
                    } else {
                        console.warn("Error response from", item.service, payload);
                    }
                }
            }
        }
    }

    onCancelled: {
        if (token == 0) {
            console.warn("All LS2 calls have been cancelled for some reason. Re-listen to server status for all");
            __listenServerStatus();
        } else if (__statusTokens.indexOf(token) != -1) {
            // Should not happen as it means someone else cancelled this unexpectedly!
            // If it really happens, we'd better to cancel others too for simplicity.
            // Be aware that we must remove the token cancelled from the array
            // to avoid an infinite loop of cancel and onCancelled.
            console.warn("A registerServerStatus call has been cancelled. Re-listen to server status for all", token);
            while (__statusTokens.length > 0)
                cancel(__statusTokens.pop());
            __listenServerStatus();
        }
    }

    onSessionIdChanged: {
        for (var i in __pendingRequests) {
            if (__pendingRequests[i].sessionId != undefined) {
                for (var key in __subscribedKeyValueMap) {
                    if (__pendingRequests[i].key == key) {
                        // To prevent the object leaks when switching sessions
                        delete __subscribedKeyValueMap[key];
                        break;
                    }
                }
            }
        }
    }
}
