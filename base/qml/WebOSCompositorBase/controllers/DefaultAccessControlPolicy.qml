// Copyright (c) 2018 LG Electronics, Inc.
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

Item {
    id: root

    property var fullscreenView
    property var overlayView
    property var launcher
    property var popupView
    property var notification
    property var keyboardView
    property var spinner

    // Access properties for UI components and key events
    // true: allow to show(default), false: disallow to show

    Connections {
        target: fullscreenView

        onContentChanged: {
            if (fullscreenView.currentItem && fullscreenView.currentItem.type == "_WEBOS_WINDOW_TYPE_RESTRICTED")
                toRestrictedMode();
            else
                reset();
        }
    }

    function toRestrictedMode() {
        console.info("AccessControl: the restricted mode is set.");
        fullscreenView.access = true;
        overlayView.access = false;
        launcher.access = false;
        popupView.access = false;
        notification.acceptAlerts = false;
        notification.acceptToasts = false;
        notification.acceptPincodePrompts = false;
        keyboardView.access = true;
        spinner.access = false;
    }

    function reset() {
        console.info("AccessControl: get back to the default access control policy.");
        fullscreenView.access = true;
        overlayView.access = true;
        launcher.access = true;
        popupView.access = true;
        notification.acceptAlerts = true;
        notification.acceptToasts = true;
        notification.acceptPincodePrompts = true;
        keyboardView.access = true;
        spinner.access = true;
    }
}
