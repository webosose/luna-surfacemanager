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
import WebOSCompositorBase 1.0

Item {
    id: root

    property var access
    property var keyController
    property var views

    // Though unused, have these to check if Settings.subscribe() works correctly
    property string country: Settings.subscribe("com.webos.settingsservice", "getSystemSettings", {"category":"option", "keys":["country"]}) || ""
    property var allConfigs: Settings.subscribe("com.webos.service.config", "getConfigs", {"configNames":["com.webos.surfacemanager.*"]}, true) || {}

    onCountryChanged: {
        console.info("com.webos.settingsservice, option, country:", country);
    }

    onAllConfigsChanged: {
        console.info("com.webos.service.config, com.webos.surfacemanager.*:", JSON.stringify(allConfigs));
    }

    Connections {
        target: views.fullscreen
        onSurfaceAdded: {
            if (views.spinner)
                views.spinner.closeView();
            if (views.keyboard)
                views.keyboard.closeView();
            if (views.notification)
                views.notification.closeView();
            if (views.popup)
                views.popup.closeView();
            if (views.launcher)
                views.launcher.closeView();
            if (views.overlay)
                views.overlay.closeView();
        }
    }

    Connections {
        target: views.overlay
        onSurfaceAdded: {
            if (views.spinner)
                views.spinner.closeView();
            if (views.keyboard)
                views.keyboard.closeView();
            if (views.notification)
                views.notification.closeView();
            if (views.popup)
                views.popup.closeView();
            if (views.launcher)
                views.launcher.closeView();
        }
    }

    Connections {
        target: views.launcher
        onOpening: {
            if (views.spinner)
                views.spinner.closeView();
            if (views.keyboard)
                views.keyboard.closeView();
            if (views.notification)
                views.notification.closeView();
            if (views.popup)
                views.popup.closeView();
            if (views.overlay)
                views.overlay.closeView();
        }
    }

    Connections {
        target: views.popup
        onSurfaceAdded: {
            if (views.spinner)
                views.spinner.closeView();
            if (views.keyboard)
                views.keyboard.closeView();
            if (views.notification)
                views.notification.closeView();
        }
    }

    Connections {
        target: views.spinner
        onOpening: {
            if (views.keyboard)
                views.keyboard.closeView();
            if (views.popup)
                views.popup.closeView();
            if (views.launcher)
                views.launcher.closeView();
            if (views.overlay)
                views.overlay.closeView();
        }
    }

    Connections {
        target: LS.applicationManager
        onAppLifeEventsChanged: {
            console.info(event, appId);
            if (event === "splash") {
                if (appId != null && (showSplash || showSpinner))
                    views.spinner.start(appId);
            } else if (event === "launch") {
                //This part should be removed once "splash" event becomes available.
                if (views.spinner && !views.spinner.isOpen && appId != null) {
                    var needSpinner = false;
                    var foregroundItems = Utils.foregroundList(root.views.children);
                    if (foregroundItems.length === 0) {
                        needSpinner = true;
                    } else if (foregroundItems[0].appId !== appId) {
                        needSpinner = true;
                    }
                    if (needSpinner)
                        views.spinner.start(appId);
                }
            } else if (event === "stop") {
                if (views.spinner && appId == views.spinner.appId)
                    views.spinner.closeView();
            }
        }
    }

    Connections {
        target: keyController
        onHomePressed: {
            if (views.notification && views.notification.active)
                views.notification.closeView();
            else if (views.launcher && !(views.notification && views.notification.modal))
                views.launcher.toggleHome();
        }
    }

    Connections {
        target: Settings.system
        onScreenRotationChanged: {
            if (Settings.system.screenRotation == "off")
                compositorWindow.updateOutputGeometry(0);
            else if (Settings.system.screenRotation == "90")
                compositorWindow.updateOutputGeometry(90);
            else if (Settings.system.screenRotation == "180")
                compositorWindow.updateOutputGeometry(180);
            else if (Settings.system.screenRotation == "270")
                compositorWindow.updateOutputGeometry(270);
        }
    }
}
