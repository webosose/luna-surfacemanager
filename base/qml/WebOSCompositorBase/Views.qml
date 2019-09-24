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
import WebOSCompositorBase 1.0

import "qrc:/WebOSCompositor"
import "file:///usr/lib/qml/WebOSCompositor"

FocusScope {
    id: root
    focus: true

    property alias views: viewsRoot

    FocusScope {
        id: compositorRoot
        focus: true

        anchors.centerIn: parent
        anchors.horizontalCenterOffset: compositorWindow.outputGeometry.x
        anchors.verticalCenterOffset: compositorWindow.outputGeometry.y
        width: compositorWindow.outputGeometry.width
        height: compositorWindow.outputGeometry.height
        rotation: compositorWindow.outputRotation
        clip: compositorWindow.outputClip

        ViewsRoot {
            id: viewsRoot
            anchors.fill: parent
        }

        Binding {
            target: compositorWindow
            property: "viewsRoot"
            value: viewsRoot
        }

        Loader {
            anchors.fill: parent
            source: Settings.local.debug.enable ? "views/debug/DebugOverlay.qml" : ""
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
