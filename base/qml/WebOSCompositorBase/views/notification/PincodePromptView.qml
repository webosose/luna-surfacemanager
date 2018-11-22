// Copyright (c) 2013-2018 LG Electronics, Inc.
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

FocusableView {
    id: root
    layerNumber: 7
    objectName: "pincodePromptView"

    property var model
    property bool enabled: true
    property int retry_num: 0

    anchors.fill: parent

    onFocusChanged: {
        pincodePromptStack.focus = focus;
    }

    Connections {
        target: pincodePromptStack
        onStateChanged: {
            switch (pincodePromptStack.state) {
            case "open":
                root.openView();
                break;
            case "closed":
                root.closeView();
                break;
            }
        }
    }

    openAnimation: SequentialAnimation {
        PropertyAnimation {
            target: pincodePromptStack
            property: "y"
            from: root.height
            to: root.height - pincodePromptStack.height
            duration: Settings.local.notification.pincodePrompt.slideAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    closeAnimation: SequentialAnimation {
        PropertyAnimation {
            target: pincodePromptStack
            property: "y"
            from: root.height - pincodePromptStack.height
            to: root.height
            // There will be no visual animation when an item
            // is removed immediately from the model.
            duration: Settings.local.notification.pincodePrompt.slideAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    NotificationStack {
        id: pincodePromptStack
        rootObjectName: root.objectName
        enabled: root.enabled

        z: 100000

        x: 0
        y: root.height - pincodePromptStack.height

        width: root.width
        height: Settings.local.notification.pincodePrompt.height

        model: root.model

        delegate: Component {
            PincodePrompt {
                id: pincodePrompt
                title : getPromptTitle(model.pincodePromptInfo.retry, model.pincodePromptInfo.promptType, model.pincodePromptInfo.title)
                retry: model.pincodePromptInfo.retry

                LayoutMirroring.enabled: Settings.l10n.isRTL
                LayoutMirroring.childrenInherit: Settings.l10n.isRTL

                z: pincodePromptStack.z - 1

                Component.onCompleted: {
                    pincodePrompt.focus = true;
                }

                onPostPassword: {
                    relayPincode(pincodePrompt.password, model.pincodePromptInfo.promptType);
                }

                onPostExitSignal: {
                    closePincodePrompt();
                }

                onRetryChanged: {
                    if (retry) {
                        retry_num++;
                        if (retry_num === 3)
                            closePincodePrompt();
                    } else {
                        retry_num = 0;
                    }
                }

                Connections {
                    target: pincodePromptStack
                    onScrimClicked: exitClicked();
                }
            }
        }
    }

    function relayPincode(password, promptType) {
        if (pincodePromptStack.state !== "open")
            return;

        if (typeof password === "undefined" || password === null)
            password = "";

        if (typeof promptType === "undefined" || promptType === null || promptType === "parental")
            promptType = "";

        notificationRoot.notificationService.closePincodePrompt({ pincode: password, closeType: "relay", mode: promptType });
    }

    function closePincodePrompt() {
        retry_num = 0;
        if (pincodePromptStack.state !== "open")
            return;

        if (notificationRoot.notificationService.connected)
            notificationRoot.notificationService.closePincodePrompt({ closeType: "close" });
        else
            root.model.clear();
    }
}
