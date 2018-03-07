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
import WebOSServices 1.0

SettingsService {
    id: root

    property QtObject settings: QtObject {
        property alias cached: root.cached
        property alias bootStatus: root.bootStatus
        property alias screenRotation: root.screenRotation
    }

    property QtObject l10n: QtObject {
        property alias tr: root.emptyString
        property bool isRTL: false
    }

    property var subscriptions: [
        // Handler should return subscribed value.
        {
            service: "com.webos.settingsservice",
            method: "getSystemSettings",
            token: 0,
            handler: handleGetSystemSettings,
            connected: false
        },
        {
            service: "com.webos.service.config",
            method: "getConfigs",
            token: 0,
            handler: handleGetConfigs,
            connected: false
        }
    ];

    function handleGetSystemSettings(response) {
        if (response.settings !== undefined) {
            for (var keys in response.settings)
                return response.settings[keys];
        }
    }

    function handleGetConfigs(response) {
        if (response.configs !== undefined) {
            for (var keys in response.configs)
                return response.configs[keys];
        }
    }

    function subscribeOnDemand(serviceName, methodName, param) {
        if (param["subscribe"] !== true)
            param["subscribe"] = true;

        var sortedParam = JSON.stringify(__sortObject(param));
        var keyGenerated = serviceName + methodName + sortedParam;

        if (__subscribedKeyValueMap[keyGenerated] == undefined) {
            // serviceName x methodName should provide only one matching item.
            var targetPrototype = subscriptions.filter(
                function (element) {
                    return (element.service === serviceName &&
                    element.method === methodName);
                }
            )[0];

            if (targetPrototype) {
                var subscribeItem = Object.create(targetPrototype);

                subscribeItem["params"] = sortedParam;
                subscribeItem["key"] = keyGenerated;

                console.info("Register subscription to " + serviceName + "/" + methodName + " " + sortedParam);
                __subscribeService(subscribeItem);
                __subscribedKeyValueMap[keyGenerated] =
                    Qt.createQmlObject("import QtQuick 2.4; QtObject { property var value }", root);
            } else {
                console.warn("Unsupported subscription: " + serviceName + "/" + methodName);
                return;
            }
        }

        return __subscribedKeyValueMap[keyGenerated].value;
    }

    property var __statusTokens: []
    property var __pendingRequests: []
    property var __subscribedRequestMap: ({})
    property var __subscribedKeyValueMap: ({})

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
        if (subscribeItem.connected) {
            subscribeItem.token = call("luna://" + subscribeItem.service,
                                       "/" + subscribeItem.method,
                                       subscribeItem.params);

            if (subscribeItem.token == 0) {
                console.warn("Failed to subscribe to", subscribeItem.service);
            } else {
                console.log("Subscribed to " + subscribeItem.service + "/" +
                    subscribeItem.method + " " + subscribeItem.params);
                __subscribedRequestMap[subscribeItem.token] = subscribeItem;
                return true;
            }
        }

        console.log("Subscription pending for " + subscribeItem.service + "/" +
            subscribeItem.method + " " + subscribeItem.params);
        __pendingRequests.push(subscribeItem);

        return false;
    }

    function __updateServerStatus(serviceName, connected) {
        // Update service status in prototype
        subscriptions.reduce(
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
                        "params: " + __subscribedRequestMap[i].params);
                    __pendingRequests.push(__subscribedRequestMap[i]);
                    cancel(i);
                    delete __subscribedRequestMap[i];
                }
            }
        }
    }

    Component.onCompleted: {
        console.log("Start subscribing to SettingsService");
        subscribe();

        var token;
        for (var item in subscriptions) {
            token = registerServerStatus(subscriptions[item].service);
            if (token != 0)
                __statusTokens.push(token);
            else
                console.error("Error on registerServerStatus for", subscriptions[item].service);
        }
    }

    onL10nLoadSucceeded:    console.info("Localization: Loaded", file);
    onL10nInstallSucceeded: console.info("Localization: Installed", file);
    onL10nLoadFailed:       console.warn("Localization: Failed to load", file);
    onL10nInstallFailed:    console.warn("Localization: Failed to install", file);
    onError:                console.warn("Localization: An error occurred,", errorText);

    onCurrentLocaleChanged: {
        if (currentLocale &&
            (currentLocale.indexOf("he-") > -1 || currentLocale.indexOf("ar-") > -1 ||
            currentLocale.indexOf("ur-") > -1 || currentLocale.indexOf("ku-") > -1 ||
            currentLocale.indexOf("fa-") > -1))
            l10n.isRTL = true;
        else
            l10n.isRTL = false;
        console.info("Localization: UI Locale changed. currentLocale: " + currentLocale + ", isRTL: " + l10n.isRTL);
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
                if (response.returnValue !== undefined && response.returnValue) {
                    if (__subscribedKeyValueMap[item.key].value = item.handler(response))
                        console.log("Value updated: " + item.key + "=" + __subscribedKeyValueMap[item.key].value);
                    else
                        console.warn("Unhandled response for", item.service, payload);
                } else {
                    console.warn("Error response from", item.service, payload);
                }
            }
        }
    }
}
