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

    property var views

    // Access properties for UI components and key events
    // true: allow to show(default), false: disallow to show

    Connections {
        target: views.fullscreen

        onContentChanged: {
            if (views.fullscreen.currentItem && views.fullscreen.currentItem.type == "_WEBOS_WINDOW_TYPE_RESTRICTED")
                toRestrictedMode();
            else
                reset();
        }
    }

    function toRestrictedMode() {
        console.info("AccessControl: the restricted mode is set.");
        for (var i = 0; i < views.children.length; i++) {
            if (views.children[i] != views.fullscreen && typeof views.children[i].access !== "undefined") {
                console.log("AccessControl: restrict access to " + views.children[i]);
                views.children[i].access = false;
            }
        }

        console.log("AccessControl: allow access only to " + views.fullscreen);
        views.fullscreen.access = true;
    }

    function reset() {
        console.info("AccessControl: get back to the default access control policy.");
        for (var i = 0; i < views.children.length; i++) {
            if (typeof views.children[i].access !== "undefined") {
                console.log("AccessControl: allow access to " + views.children[i]);
                views.children[i].access = true;
            }
        }
    }
}
