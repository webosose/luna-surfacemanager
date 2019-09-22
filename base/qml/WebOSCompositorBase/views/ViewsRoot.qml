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

    property alias fullscreen: fullscreenViewId
    property alias overlay: overlayViewId
    property alias launcher: launcherId
    property alias popup: popupViewId
    property alias notification: notificationViewId
    property alias keyboard: keyboardViewId
    property alias spinner: spinnerId
    property alias systemUi: systemUiViewId
    property alias launcherHotspot: launcherHotspotId

    // No suffix for objects for the display 0
    // not to break the existing TAS scripts
    readonly property string suffix: compositorWindow.displayId > 0 ? compositorWindow.displayId : ""

    FullscreenView {
        id: fullscreenViewId
        objectName: "fullscreenView" + suffix
        anchors.fill: parent
        model: FullscreenWindowModel {}
    }

    OverlayView {
        id: overlayViewId
        objectName: "overlayView" + suffix
        anchors.fill: parent
        model: OverlayWindowModel {}
    }

    Launcher {
        id: launcherId
        objectName: "launcher" + suffix
        anchors.fill: parent
    }

    PopupView {
        id: popupViewId
        objectName: "popupView" + suffix
        model: PopupWindowModel {}
    }

    NotificationView {
        id: notificationViewId
        objectName: "notificationView" + suffix
        anchors.fill: parent
    }

    KeyboardView {
        id: keyboardViewId
        objectName: "keyboardView" + suffix
        model: KeyboardWindowModel {}
    }

    Spinner {
        id: spinnerId
        objectName: "spinner" + suffix
        anchors.fill: parent
    }

    SystemUIView {
        id: systemUiViewId
        objectName: "systemUiView" + suffix
        model: SystemUIWindowModel {}
    }

    TouchHotspot {
        id: launcherHotspotId
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: Settings.local.launcher.hotspotThickness
        threshold: Settings.local.launcher.hotspotThreshold
        reverse: true
    }
}
