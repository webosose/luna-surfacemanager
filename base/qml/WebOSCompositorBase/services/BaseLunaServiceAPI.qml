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
import WebOSCoreCompositor 1.0
import WebOSServices 1.0
import WebOSCompositorBase 1.0

import "qrc:/WebOSCompositor"
import "file:///usr/lib/qml/WebOSCompositor"
import "base"

Service {
    id: root

    methods: defaultMethods

    property var views

    readonly property var defaultMethods: ["closeByAppId", "getForegroundAppInfo", "captureCompositorOutput"]

    readonly property ForegroundAppInfoMgr foregroundAppInfoMgr: ForegroundAppInfoMgr {
        foregroundItems: compositor.foregroundItems
        onForegroundAppInfoChanged: {
            root.pushSubscription("getForegroundAppInfo");
        }
    }

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

    function getForegroundAppInfo(param) {
        var ret = {};

        console.info("LS2 method handler is called with param: " + JSON.stringify(param));

        ret.foregroundAppInfo = foregroundAppInfoMgr.getForegroundAppInfo();

        return JSON.stringify(ret);
    }

    function captureCompositorOutput(param) {
        var path = "";
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

        if (param.appId) {
            target = Utils.surfaceForApplication(param.appId);
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
            var result = Utils.capture(path, target, format);
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
                }
                console.warn("errorCode: " + ret.errorCode + ", errorText: " + ret.errorText);
                return JSON.stringify(ret);
            }
        }

        return JSON.stringify(ret);
    }
}
