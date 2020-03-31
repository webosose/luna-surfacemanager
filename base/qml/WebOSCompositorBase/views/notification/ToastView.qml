// Copyright (c) 2016-2020 LG Electronics, Inc.
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

BaseView {
    id: root
    objectName: "toastView"

    property var model
    property bool enabled: true

    anchors.fill: parent

    Connections {
        target: toastStack
        onStateChanged: {
            switch (toastStack.state) {
            case "open":
                root.openView();
                break;
            case "closed":
                root.closeView();
                break;
            }
        }
    }

    // PauseAnimation to sync the view transition with
    // the visual animation defined in Toast.qml.
    openAnimation: SequentialAnimation {
        PauseAnimation {
            duration: toastStack.slideAnimationDuration
        }
    }

    closeAnimation: SequentialAnimation {
        PauseAnimation {
            duration: toastStack.slideAnimationDuration
        }
    }

    NotificationStack {
        id: toastStack
        rootObjectName: root.objectName
        enabled: root.enabled
        scrimEnabled : false

        x: root.width - width
        y: 0
        width: Settings.local.notification.toast.width
        height: Settings.local.notification.toast.height

        timeout: Settings.local.notification.toast.timeout

        property int slideAnimationDuration: Settings.local.notification.toast.slideAnimationDuration

        model: root.model

        delegate: Component {
            Toast {
                anchors.top: parent.top
                message: model.message
                iconUrl: model.iconUrl
                LayoutMirroring.enabled: Settings.l10n.isRTL
                LayoutMirroring.childrenInherit: Settings.l10n.isRTL
                slideAnimationDuration: toastStack.slideAnimationDuration

                Component.onCompleted: {
                    toastStack.restartTimer();
                    toastStack.timeoutEnabled = true;
                }

                onOpened: {
                    root.contentChanged();
                }

                onClosed: {
                    if (ListView.view.model.count > 0)
                        ListView.view.model.remove(ListView.view.model.count-1);
                    root.contentChanged();
                }

                onActivated: {
                    if (model.action && Object.keys(model.action).length > 0)
                         LS.adhoc.call(model.action.serviceURI, model.action.serviceMethod, JSON.stringify(model.action.launchParams));
                    if (ListView.view.model.count > 0)
                        ListView.view.model.remove(ListView.view.model.count-1);
                }

                onContainsMouseChanged: {
                    toastStack.timeoutEnabled = !containsMouse;
                    toastStack.restartMasterTimer();
                }
            }
        }

        onTimeoutTriggered: {
            root.close();
            if (root.model.count === 0)
                toastStack.state = "closed";
            toastStack.stopMasterTimer();
        }
    }

    function close() {
        if (root.model.count > 0)
            root.model.clear();
    }

    function restartTimer() {
        toastStack.restartTimer();
    }
}
