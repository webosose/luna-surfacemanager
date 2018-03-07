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

import "../WebOSCompositor"

FocusScope {
    id: root
    focus: true

    property var fullscreen: fullscreenViewId
    property var overlay: overlayViewId
    property var launcher: launcherId
    property var popup: popupViewId
    property var notification: notificationViewId
    property var keyboardView: keyboardViewId
    property var spinner: spinnerId

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

        FullscreenView {
            id: fullscreenViewId
            objectName: "fullscreenView"
            anchors.fill: parent
            model: FullscreenWindowModel {}
        }

        OverlayView {
            id: overlayViewId
            objectName: "overlayView"
            anchors.fill: parent
            model: OverlayWindowModel {}
        }

        Launcher {
            id: launcherId
            objectName: "launcher"
            anchors.fill: parent
        }

        PopupView {
            id: popupViewId
            objectName: "popupView"
            model: PopupWindowModel {}
        }

        NotificationView {
            id: notificationViewId
            objectName: "notificationView"
            anchors.fill: parent
        }

        KeyboardView {
            id: keyboardViewId
            objectName: "keyboardView"
            model: KeyboardWindowModel {}
        }

        Spinner {
            id: spinnerId
            objectName: "spinner"
            anchors.fill: parent
        }

        Loader {
            id: debugOverlayId
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
