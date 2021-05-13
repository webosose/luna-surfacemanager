// Copyright (c) 2013-2021 LG Electronics, Inc.
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

FocusScope {
    id: root

    property url iconUrl
    property string title
    property string message

    property string acceptButtonText: qsTr("Yes") + Settings.l10n.tr
    property string rejectButtonText: qsTr("No") + Settings.l10n.tr

    property bool modal: false
    property bool showRejectButton: false
    property var closeAction

    width: parent.width
    height: Settings.local.notification.alert.height

    signal acceptClicked()
    signal rejectClicked()
    signal closeClicked()

    onFocusChanged: {
        acceptButton.focus = focus;
    }

    Rectangle {
        id: alertBackground
        anchors.fill: parent
        color: Settings.local.notification.alert.backgroundColor

        MouseArea {
            id: mouseBlocker
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onClicked: (mouse) => {}
            onEntered: {}
            onDoubleClicked: (mouse) => {}
            onWheel: (wheel) => {}
            onPressed: (mouse) => {}
            onPressAndHold: (mouse) => {}
            hoverEnabled: true
        }
    }

    Item {
        width: parent.width
        height: parent.height
        anchors.centerIn: parent

        Image {
            id: alertIcon
            source: root.iconUrl ? root.iconUrl : ""
            visible: root.iconUrl.toString() !== ""
            width: Settings.local.notification.alert.iconSize
            height: width
            fillMode: Image.PreserveAspectFit
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: Settings.local.notification.alert.iconMargins
            anchors.rightMargin: Settings.local.notification.alert.iconMargins
            anchors.topMargin: Settings.local.notification.alert.iconMargins
            smooth: true
        }

        Text {
            id: alertText
            text: root.title
            color: Settings.local.notification.alert.foregroundColor

            anchors.top: parent.top
            anchors.topMargin: Settings.local.notification.alert.textMargins
            anchors.left: parent.left
            anchors.leftMargin: Settings.local.notification.alert.textMargins + (alertIcon.visible ? Settings.local.notification.alert.textIconWidth : 0)
            anchors.right: closeButton.left
            anchors.rightMargin: Settings.local.notification.alert.textMargins

            font.family: Settings.local.notification.alert.headerFont
            font.pixelSize: Settings.local.notification.alert.fontSize
            font.capitalization: Font.AllUppercase
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WrapAnywhere
            elide: Text.ElideRight

            maximumLineCount: 1
        }

        Text {
            id: alertDescription
            text: root.message
            color: Settings.local.notification.alert.foregroundColor

            anchors.top: alertText.bottom
            anchors.left: parent.left
            anchors.topMargin: Settings.local.notification.alert.descriptionTopMargin
            anchors.leftMargin: alertText.anchors.leftMargin
            width: Settings.local.notification.alert.textWidth + (alertIcon.visible ? 0 : Settings.local.notification.alert.textIconWidth) + (root.showRejectButton? 0 : rejectButton.width)

            font.family: Settings.local.notification.alert.fontName
            font.weight: Settings.local.notification.alert.fontWeight
            font.pixelSize: Settings.local.notification.alert.descriptionFontSize
            horizontalAlignment: Text.AlignLeft
            lineHeightMode: Text.FixedHeight
            lineHeight: Settings.local.notification.alert.descriptionLineHeight
            wrapMode: Text.Wrap
            elide: Text.ElideRight

            maximumLineCount: 3
        }

        Row {
            id: buttonRow
            anchors.right: parent.right
            anchors.rightMargin: Settings.local.notification.alert.buttonMargins
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Settings.local.notification.alert.buttonMargins
            spacing: Settings.local.notification.alert.buttonMargins / 2

            Button {
                id: acceptButton
                text: root.acceptButtonText
                KeyNavigation.right: rejectButton.visible ? rejectButton : null
                KeyNavigation.up: closeButton
                onClicked: root.acceptClicked();
                focus: true
            }

            Button {
                id: rejectButton
                visible: root.showRejectButton
                text: root.rejectButtonText
                KeyNavigation.left: acceptButton
                KeyNavigation.up : closeButton
                onClicked: root.rejectClicked();
            }
        }

        CloseButton {
            id: closeButton
            visible: !root.modal
            anchors.top: parent.top
            anchors.right: parent.right

            KeyNavigation.down: acceptButton

            onClicked: root.closeClicked();
        }
    }
}
