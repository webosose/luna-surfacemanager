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

FocusableView {
    id: root
    layerNumber: 8
    objectName: "alertView"

    property var model
    property bool enabled: true
    property int modalAlertCount: 0
    property bool modalAlertsVisible: modalAlertCount && enabled

    anchors.fill: parent

    onFocusChanged: {
        alertStack.focus = focus;
    }

    Connections {
        target: alertStack
        onStateChanged: {
            switch (alertStack.state) {
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
            target: alertStack
            property: "y"
            from: root.height
            to: root.height - alertStack.height
            duration: Settings.local.notification.alert.slideAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    closeAnimation: SequentialAnimation {
        PropertyAnimation {
            target: alertStack
            property: "y"
            from: root.height - alertStack.height
            to: root.height
            // There will be no visual animation when an item
            // is removed immediately from the model.
            duration: Settings.local.notification.alert.slideAnimationDuration
            easing.type: Easing.InOutCubic
        }
    }

    NotificationStack {
        id: alertStack
        rootObjectName: root.objectName
        enabled: root.enabled

        z: 100000

        x: 0
        y: root.height - alertStack.height

        width: root.width
        height: Settings.local.notification.alert.height

        model: root.model

        delegate: Component {
            Alert {
                message: model.alertInfo.message
                iconUrl: model.alertInfo.iconUrl ? model.alertInfo.iconUrl : ""
                title: model.alertInfo.title || ""
                modal: model.alertInfo.modal
                showRejectButton: model.alertInfo.buttons.length > 1
                acceptButtonText: model.alertInfo.buttons[0].label
                rejectButtonText: showRejectButton ? model.alertInfo.buttons[1].label : ""
                z: modal ? (index*-1) : (-10000 - index)
                LayoutMirroring.enabled: Settings.l10n.isRTL
                LayoutMirroring.childrenInherit: Settings.l10n.isRTL
                closeAction: model.alertInfo.onCloseAction
                focus: root.modalAlertCount > 0 ? z === 0 : z === -10000

                Component.onCompleted: {
                    if (modal) {
                        alertStack.modal = modal;
                        root.modalAlertCount++;
                    }
                }

                onAcceptClicked: {
                    closeAction = null;
                    if (model.alertInfo.buttons[0].action)
                         LS.adhoc.call(model.alertInfo.buttons[0].action.serviceURI, model.alertInfo.buttons[0].action.serviceMethod, JSON.stringify(model.alertInfo.buttons[0].action.launchParams));
                    if (ListView.view.model.count > 0)
                        ListView.view.model.remove(index);
                }

                onRejectClicked: {
                    closeAction = null;
                    if (model.alertInfo.buttons[1].action)
                         LS.adhoc.call(model.alertInfo.buttons[1].action.serviceURI, model.alertInfo.buttons[1].action.serviceMethod, JSON.stringify(model.alertInfo.buttons[1].action.launchParams));
                    if (ListView.view.model.count > 0)
                        ListView.view.model.remove(index);
                }

                onCloseClicked: {
                    if (ListView.view.model.count > 0)
                        ListView.view.model.remove(index);
                }

                ListView.onRemove: {
                   if (closeAction && Object.keys(closeAction).length > 0)
                       LS.adhoc.call(closeAction.serviceURI, closeAction.serviceMethod, JSON.stringify(closeAction.launchParams));
                   if (modal)
                       root.modalAlertCount--;
                   alertStack.modal = root.modalAlertCount > 0;
                }

                Connections {
                    target: alertStack
                    onScrimClicked: if (!alertStack.modal) closeClicked();
                }
            }
        }
    }

    function close(includesModal) {
        // NOTE: A modal alert is always placed on any non-modal alerts.
        //       If the param is undefined, non-modal alerts are only cleared.
        if (includesModal || !modalAlertsVisible)
            root.model.clear();
    }
}
