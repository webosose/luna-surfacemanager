// Copyright (c) 2017-2024 LG Electronics, Inc.
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
import WebOSCompositor 1.0
import WebOSCompositorExtended 1.0

FocusScope {
    id: root
    focus: true

    property alias views: viewsRoot

    FocusScope {
        id: compositorRoot
        focus: true

        // x, y offset will not work when anchors.centerIn is active
        // Namely, in rotation case, the x,y offset will be ignored.
        anchors.centerIn: rotation != 0 ? parent : undefined

        x: compositorWindow.outputGeometry.x
        y: compositorWindow.outputGeometry.y
        width: compositorWindow.outputGeometry.width
        height: compositorWindow.outputGeometry.height
        rotation: compositorWindow.outputRotation
        clip: compositorWindow.outputClip

        property var compositorGeometryConfig: Settings.subscribe("com.webos.service.config", "getConfigs", {"configNames":["com.webos.surfacemanager.compositorGeometry"]})
        onCompositorGeometryConfigChanged: {
            console.info("compositorGeometryConfig", compositorGeometryConfig);
            if (compositorGeometryConfig) {
                console.info("Set compositorWindow's geometryConfig to ", compositorGeometryConfig);
                compositorWindow.geometryConfig = compositorGeometryConfig
            }
        }

        property var geometryAnimationConfig: Settings.subscribe("com.webos.service.config", "getConfigs", {"configNames":["com.webos.surfacemanager.enableGeometryAnimation"]})
        property bool geometryAnimationEnabled: (typeof(geometryAnimationConfig) !== 'undefined' && geometryAnimationConfig == true) ? true : false

        Behavior on x {
            enabled: compositorRoot.geometryAnimationEnabled
            PropertyAnimation { duration: 1500 }
        }
        Behavior on y {
            enabled: compositorRoot.geometryAnimationEnabled
            PropertyAnimation { duration: 1500 }
        }
        Behavior on width {
            enabled: compositorRoot.geometryAnimationEnabled
            PropertyAnimation { duration: 1500 }
        }
        Behavior on height {
            enabled: compositorRoot.geometryAnimationEnabled
            PropertyAnimation { duration: 1500 }
        }

        ViewsRoot {
            id: viewsRoot
            anchors.fill: parent
            property var foregroundItems: Utils.foregroundList(viewsRoot.children);
        }

        Binding {
            target: compositorWindow
            property: "viewsRoot"
            value: viewsRoot
        }

        Loader {
            anchors.fill: parent
            source: Settings.local.devel && Settings.local.debug.enable ? "views/debug/DebugOverlay.qml" : ""
        }
    }

    ScreenFreezer {
        visible: compositorWindow.outputGeometryPending
        target: compositorRoot // should be a sibling
        anchors.centerIn: target

        Component.onCompleted: {
            compositorWindow.outputGeometryPendingInterval = Settings.local.compositor.geometryPendingInterval;
        }
    }
}
