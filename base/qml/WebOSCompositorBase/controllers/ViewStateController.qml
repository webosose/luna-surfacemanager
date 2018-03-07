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

    property var fullscreenView
    property var overlayView
    property var launcher
    property var popupView
    property var notification
    property var keyboardView
    property var spinner

    property var keyFilter

    property var country: Settings.subscribe("com.webos.settingsservice", "getSystemSettings", {"category":"option", "keys":["country"]});

    onCountryChanged: {
        console.info("com.webos.settingsservice, option, country:", country);
    }

    Connections {
        target: fullscreenView
        onSurfaceAdded: {
            if (spinner)
                spinner.closeView();
            if (keyboardView)
                keyboardView.closeView();
            if (notification)
                notification.closeView();
            if (popupView)
                popupView.closeView();
            if (launcher)
                launcher.closeView();
            if (overlayView)
                overlayView.closeView();
        }
    }

    Connections {
        target: overlayView
        onSurfaceAdded: {
            if (spinner)
                spinner.closeView();
            if (keyboardView)
                keyboardView.closeView();
            if (notification)
                notification.closeView();
            if (popupView)
                popupView.closeView();
            if (launcher)
                launcher.closeView();
        }
    }

    Connections {
        target: launcher
        onOpening: {
            if (spinner)
                spinner.closeView();
            if (keyboardView)
                keyboardView.closeView();
            if (notification)
                notification.closeView();
            if (popupView)
                popupView.closeView();
            if (overlayView)
                overlayView.closeView();
        }
    }

    Connections {
        target: popupView
        onSurfaceAdded: {
            if (spinner)
                spinner.closeView();
            if (keyboardView)
                keyboardView.closeView();
        }
    }

    Connections {
        target: spinner
        onOpening: {
            if (keyboardView)
                keyboardView.closeView();
            if (popupView)
                popupView.closeView();
            if (launcher)
                launcher.closeView();
            if (overlayView)
                overlayView.closeView();
        }
    }

    Connections {
        target: LS.applicationManager
        onAppLifeStatusChanged: {
            console.info(status, appId, extraInfo);
            if (status === "launching") {
                var needSpinner = false;
                try {
                    var extra = JSON.parse(extraInfo);
                    if (!extra.noSplash || extra.spinner)
                        needSpinner = true;
                } catch (e) {
                    console.warn("extraInfo in getAppLifeStatus is absent");
                    needSpinner = true;
                }
                if (spinner && appId != null && needSpinner)
                    spinner.start(appId);
            } else if (status === "stop") {
                if (spinner && appId == spinner.appId)
                    spinner.closeView();
            }
        }
    }

    Connections {
        target: keyFilter
        onHomePressed: {
            if (notification && notification.active)
                notification.closeView();
            else if (launcher && !(notification && notification.modal))
                launcher.toggleHome();
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
