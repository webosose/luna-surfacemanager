// Copyright (c) 2017-2021 LG Electronics, Inc.
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
import WebOSCoreCompositor 1.0
import WebOSServices 1.0
import WebOSCompositorBase 1.0
import WebOSCompositor 1.0
import "base"

Service {
    id: root

    methods: defaultMethods

    property var views
    property var controllers

    readonly property var defaultMethods: ["closeByAppId", "getForegroundAppInfo", "captureCompositorOutput"]

    function closeByAppId(param) {
        var ret = {};

        console.info("LS2 method handler is called with param: " + JSON.stringify(param));

        // There is no clear spec defined for a case
        // where given item is not a fullscreen surface.
        // Historically however, it is expected to behave the same
        // to com.webos.applicationManager/closeByAppId.
        var item = Utils.surfaceForApplication(param.id);
        if (item && !item.fullscreen) {
            LS.adhoc.call("luna://com.webos.applicationManager", "/closeByAppId",
                "{\"id\":\"" + param.id + "\"}");
        } else if (item && !item.isProxy()) {
            compositor.closeWindowKeepItem(item);
        } else {
            ret.errorCode = 1;
            ret.errorText = "\"" + param.id + "\" is not running";
            console.warn("errorCode: " + ret.errorCode + ", errorText: " + ret.errorText);
        }

        return JSON.stringify(ret);
    }

    readonly property ForegroundAppInfoMgr foregroundAppInfoMgr: ForegroundAppInfoMgr {}

    function getForegroundAppInfo(param) {
        function __replySubscription() {
            root.pushSubscription("getForegroundAppInfo", "", "getForegroundAppInfo_response");
        }

        function __subscribe() {
            if (root.subscribersCount("getForegroundAppInfo") == 0) {
                root.foregroundAppInfoMgr.foregroundAppInfoChanged.connect(__replySubscription);
                root.subscriptionAboutToCancel.connect(__unsubscribe);
            }
        }

        function __unsubscribe(method) {
            // Count decreases after subscriptionAboutToCancel is handled.
            // So it is the last subscription being cancelled if the count is 1.
            if (method == "getForegroundAppInfo" && root.subscribersCount("getForegroundAppInfo") == 1) {
                root.foregroundAppInfoMgr.foregroundAppInfoChanged.disconnect(__replySubscription);
                root.subscriptionAboutToCancel.disconnect(__unsubscribe);
            }
        }

        if (param.subscribe !== undefined && typeof param.subscribe == "boolean" && param.subscribe)
            __subscribe();

        return getForegroundAppInfo_response(param);
    }

    function getForegroundAppInfo_response(param) {
        var ret = {};

        console.info("LS2 method handler is called with param: " + JSON.stringify(param));

        ret.foregroundAppInfo = foregroundAppInfoMgr.getForegroundAppInfo();

        return JSON.stringify(ret);
    }

    function captureCompositorOutput(param) {
        var path = "";
        var window = compositor.windows[0];
        var target = null;
        var format = "";
        var ret = {};

        console.info("LS2 method handler is called with param: " + JSON.stringify(param));

        if (param.output && (typeof param.output === 'string' || param.output instanceof String)) {
            path = param.output;
        } else {
            if (!param.output) {
                ret.errorCode = 101;
                ret.errorText = "ERR_MISSING_OUTPUT";
            } else {
                ret.errorCode = 102;
                ret.errorText = "ERR_INVALID_PATH";
            }
            console.warn("errorCode: " + ret.errorCode + ", errorText: " + ret.errorText);
            return JSON.stringify(ret);
        }

        if (param.displayId !== undefined) {
            if (typeof param.displayId === 'number' && param.displayId >= 0 && param.displayId < compositor.windows.length) {
                window = compositor.windows[param.displayId];
            } else {
                ret.errorCode = 106;
                ret.errorText = "ERR_INVALID_DISPLAY";
                console.warn("errorCode: " + ret.errorCode + ", errorText: " + ret.errorText);
                return JSON.stringify(ret);
            }
        }

        if (param.appId) {
            target = Utils.surfaceForApplication(param.appId, param.displayId);
            if (!target) {
                ret.errorCode = 103;
                ret.errorText = "ERR_NO_SURFACE";
                console.warn("errorCode: " + ret.errorCode + ", errorText: " + ret.errorText);
                return JSON.stringify(ret);
            }
        }

        if (!param.format) {
            console.warn("Format is missing, set to default format type: BMP");
            format = "BMP";
        } else if (param.format == "BMP" || param.format == "JPG" || param.format == "PNG") {
            format = param.format;
        } else {
            ret.errorCode = 104;
            ret.errorText = "ERR_INVALID_FORMAT";
            console.warn("errorCode: " + ret.errorCode + ", errorText: " + ret.errorText);
            return JSON.stringify(ret);
        }

        if (!ret.errorCode) {
            var result = Utils.capture(path, target, format, window);
            if (result == ScreenShot.SUCCESS) {
                var size = Utils.capturedSize();
                ret.output = path
                ret.resolution = size.width + "x" + size.height;
                ret.format = format;
            } else {
                switch (result) {
                case ScreenShot.INVALID_PATH:
                    ret.errorCode = 102;
                    ret.errorText = "ERR_INVALID_PATH";
                    break;
                case ScreenShot.NO_SURFACE:
                    ret.errorCode = 103;
                    ret.errorText = "ERR_NO_SURFACE";
                    break;
                case ScreenShot.UNABLE_TO_SAVE:
                    ret.errorCode = 105;
                    ret.errorText = "ERR_UNABLE_TO_SAVE";
                    break;
                case ScreenShot.HAS_SECURED_CONTENT:
                    ret.errorCode = 107;
                    ret.errorText = "ERR_HAS_SECURED_CONTENT";
                    break;
                }
                console.warn("errorCode: " + ret.errorCode + ", errorText: " + ret.errorText);
                return JSON.stringify(ret);
            }
        }

        return JSON.stringify(ret);
    }
}
