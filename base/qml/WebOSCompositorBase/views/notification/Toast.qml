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
import WebOS.Global 1.0
import WebOSCompositorBase 1.0

FocusScope {
    id: root

    property url iconUrl
    property string message
    property bool containsMouse: mouseArea.containsMouse
    property int slideAnimationDuration: Settings.local.notification.toast.slideAnimationDuration

    signal activated()
    signal opened()
    signal closed()
    signal entered()

    width: Settings.local.notification.toast.width
    height: Settings.local.notification.toast.height +
            Math.max(_text.lineCount - Settings.local.notification.toast.lineLimitNormal, 0) *
            Settings.local.notification.toast.textLineHeight;

    function _expansionNeeded() {
        return _text.lineCount > Settings.local.notification.toast.lineLimitNormal;
    }

    FocusScope {
        id: toast
        width: parent.width
        height: parent.height
        y: -height
        enabled: !alertView.modalAlertsVisible

        Rectangle {
            id: toastBackground
            anchors.fill: parent
            color: Settings.local.notification.toast.backgroundColor
        }

        WebOSMouseArea {
            id: mouseArea
            cursorVisible: notificationRoot.cursorVisible
            z: 10
            enabled: true
            hoverEnabled: enabled
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: closeButton.left
            propagateComposedEvents: true
            onClicked: root.activated()
        }

        Image {
            id: toastIcon
            source: root.iconUrl ? root.iconUrl : ""
            width: Settings.local.notification.toast.iconSize
            height: Settings.local.notification.toast.iconSize
            fillMode: Image.PreserveAspectFit
            anchors.left: toastBackground.left
            anchors.leftMargin: Settings.local.notification.toast.margins
            anchors.verticalCenter: parent.verticalCenter
            smooth: true
        }

        Text {
            id: _text
            text: root.message
            color: mouseArea.containsMouse ?  Settings.local.notification.toast.activatedColor : Settings.local.notification.toast.foregroundColor

            anchors.left: toastIcon.right
            anchors.right: closeButton.left
            anchors.leftMargin: Settings.local.notification.toast.margins
            anchors.rightMargin: Settings.local.notification.toast.margins

            anchors.verticalCenter: _expansionNeeded() ? undefined : parent.verticalCenter
            anchors.top: _expansionNeeded() ? parent.top : undefined
            anchors.topMargin: _expansionNeeded() ? (Settings.local.notification.toast.height - toastIcon.height) / 2 : 0

            font.family: Settings.local.notification.toast.fontName
            font.weight: Settings.local.notification.toast.fontWeight
            font.pixelSize: Settings.local.notification.toast.fontSize
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            lineHeightMode: Text.FixedHeight
            lineHeight: 40
            wrapMode: Text.Wrap
            elide: Text.ElideRight

            maximumLineCount: Settings.local.notification.toast.lineLimitExpandable
        }

        CloseButton {
            id: closeButton
            z: 100
            enabled: true
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            onClicked: root.closed();
        }
    }

    Component.onCompleted: root.state = "open";

    state: "closed"
    states: [
        State {
            name: "open"
        },
        State {
            name: "closed"
        }
    ]

    transitions: [
        Transition {
            from:  "closed"
            to: "open"
            SequentialAnimation {
                NumberAnimation {
                    from: -toast.height
                    to: 0
                    target: toast
                    properties: "y"
                    duration: root.slideAnimationDuration
                    easing.type: Easing.InOutCubic
                }
                ScriptAction {
                    script: root.opened();
                }
            }
        },
        Transition {
            from:  "open"
            to: "closed"
            SequentialAnimation {
                NumberAnimation {
                    from: 0
                    to: -toast.height
                    target: toast
                    properties: "y"
                    duration: root.slideAnimationDuration
                    easing.type: Easing.InOutCubic
                }
                ScriptAction {
                    script: root.closed();
                }
            }
        }
    ]
}
