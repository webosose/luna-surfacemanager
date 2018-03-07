// Copyright (c) 2016-2018 LG Electronics, Inc.
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

import "../../../WebOSCompositor"

Item {
    id: notificationRoot

    property bool active: alertActive || pincodePromptActive // Exclude toast
    property bool modal: alertView.modalAlertsVisible

    property bool acceptAlerts: true
    property bool acceptToasts: true
    property bool acceptPincodePrompts: true

    property bool alertActive: false
    property bool pincodePromptActive: false

    readonly property int alertCount: notificationService.alertModel.count

    property bool cursorVisible: compositorWindow.cursorVisible

    anchors.fill: parent

    property NotificationService notificationService: NotificationService {
        id: notificationService
        acceptAlerts: notificationRoot.acceptAlerts
        acceptToasts: notificationRoot.acceptToasts
        acceptPincodePrompts: notificationRoot.acceptPincodePrompts
    }

    function closeView() {
        // Default close action
        closeNotifications({ alerts: true, pincodePrompt: true });
    }

    function closeNotifications(params) {
        if (params.pincodePrompt && pincodePromptActive)
            pincodePromptView.closePincodePrompt();
        if (params.alerts && !params.modalAlerts && alertActive) // no need to call it twice
            alertView.close(false); // false == do not include Modal
        if (params.modalAlerts && (alertActive || modal))
            alertView.close(true); // true == include modal
        if (params.toasts && toastCount > 0)
            toastView.close();
    }

    PincodePromptView {
        id: pincodePromptView
        model: notificationService.pincodePromptModel
        enabled: notificationRoot.acceptPincodePrompts

        onOpening: {
            if (notificationRoot.alertActive)
                alertView.close(false);
        }

        onOpened: {
            pincodePromptView.requestFocus();
            notificationRoot.pincodePromptActive = true;
        }

        onClosed: {
            pincodePromptView.releaseFocus();
            notificationRoot.pincodePromptActive = false;
        }

        Connections {
            target: notificationService
            onConnectedChanged: {
                // Close if disconnected
                if (!notificationService.connected)
                    pincodePromptView.closePincodePrompt();
            }
        }
    }

    AlertView {
        id: alertView
        model: notificationService.alertModel
        enabled: notificationRoot.acceptAlerts

        onOpening: {
            if (notificationRoot.pincodePromptActive)
                pincodePromptView.closePincodePrompt();
        }

        onOpened: {
            notificationRoot.alertActive = true;
            alertView.requestFocus();
        }

        onClosed: {
            alertView.releaseFocus();
            notificationRoot.alertActive = false;
        }
    }

    ToastView {
        id: toastView
        model: notificationService.toastModel
        enabled: notificationRoot.acceptToasts

        Connections {
            target: notificationService
            onRestartToastTimer: toastView.restartTimer();
        }
    }
}
