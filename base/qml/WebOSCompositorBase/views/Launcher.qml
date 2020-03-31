// Copyright (c) 2017-2020 LG Electronics, Inc.
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
import Eos.Window 0.1
import Eos.Controls 0.1
import WebOS.Global 1.0
import WebOSServices 1.0
import WebOSCompositorBase 1.0
import WebOSCompositor 1.0

FocusableView {
    id: root
    layerNumber: 4

    onOpening: root.requestFocus();
    onClosed: root.releaseFocus();

    LaunchPointsModel {
        id: launchPointsModel
        appId: LS.appId
    }

    function launch(id, params) {
        Utils.performanceLog.time("APP_LAUNCH", {"APP_ID": id});
        LS.adhoc.call("luna://com.webos.applicationManager", "/launch",
            "{\"id\":\"" + id + "\", \"params\":" + JSON.stringify(params) + "}");
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        enabled: root.isOpen
        visible: enabled
        hoverEnabled: enabled
        onClicked: {
            if (mouse.button == Qt.LeftButton)
                root.closeView();
        }
    }

    Rectangle {
        id: background
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: -width
        width: Settings.local.launcher.width
        color: Settings.local.launcher.backgroundColor
        opacity: Settings.local.launcher.opacity

        AnimatedImage {
            anchors.centerIn: parent
            source: Settings.local.imageResources.spinner
            visible: launchPointsModel.status != ServiceModel.Ready
            playing: visible
        }

        FocusScope {
            focus: true
            anchors.fill: parent
            visible: launchPointsModel.status == ServiceModel.Ready

            List {
                id: listArea
                focus: true
                anchors.fill: parent
                anchors.leftMargin: Settings.local.launcher.defaultMargin
                KeyNavigation.right: settingsButton
                model: launchPointsModel
                delegate: ListItem {
                    text: title
                    iconSource: icon
                    onTriggered: checked = false; // not to be checked on clicked
                    onClicked: {
                        launch(id, typeof params === "undefined" ? {} : params);
                        root.closeView();
                    }
                    WebOSMouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorVisible: compositorWindow.cursorVisible
                        onClicked: parent.clicked();
                        onEntered: {
                            listArea.currentIndex = index;
                            listArea.currentItem.focus = true;
                        }
                    }
                }

                Button {
                    id: settingsButton
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.topMargin: Settings.local.launcher.defaultMargin
                    anchors.rightMargin: Settings.local.launcher.defaultMargin
                    KeyNavigation.left: listArea.currentItem
                    iconSource: Settings.local.imageResources.settings
                    implicitWidth: Settings.local.launcher.settingsIconSize
                    implicitHeight: Settings.local.launcher.settingsIconSize
                    onClicked: {
                        launch("com.palm.app.settings", {});
                        root.closeView();
                    }
                }
            }
        }
    }

    openAnimation: SequentialAnimation {
        PropertyAnimation {
            target: background
            property: "anchors.rightMargin"
            to: 0
            duration: Settings.local.launcher.slideAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    closeAnimation: SequentialAnimation {
        PropertyAnimation {
            target: background
            property: "anchors.rightMargin"
            to: -background.width
            duration: Settings.local.launcher.slideAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    function toggleHome() {
        if (root.access) {
            if (!root.isTransitioning) {
                if (root.isOpen)
                    root.closeView();
                else
                    root.openView();
            }
        } else {
            console.warn("AccessControl: Launcher is restricted by the access control policy.");
        }
    }
}
