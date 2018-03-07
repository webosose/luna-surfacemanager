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

PincodePromptBaseButton {
    id: root

    property color numPadNormalFontColor: Settings.local.notification.pincodePrompt.numPadNormalFontColor
    property color numPadFontActiveColor: Settings.local.notification.pincodePrompt.numPadFontActiveColor
    property color numPadFontDisabledColor: Settings.local.notification.pincodePrompt.numPadFontDisabledColor
    property string buttonTextFontFamilyName: Settings.local.notification.pincodePrompt.numPadFontFamilyName

    width: Settings.local.notification.pincodePrompt.numPadButtonWidth
    height: Settings.local.notification.pincodePrompt.numPadButtonHeight

    Rectangle {
        id: background
        anchors.fill: parent
        border.color: root.pressed ? Settings.local.notification.pincodePrompt.numPadActiveColor : color
        border.width: Settings.local.notification.pincodePrompt.numPadBorderWidth
        antialiasing: true
        color: root.highlight && !root.pressed ? Settings.local.notification.pincodePrompt.numPadActiveColor : Settings.local.notification.pincodePrompt.numPadColor
    }

    Text {
        id: buttonText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        text: root.text
        color: !root.enabled ? numPadFontDisabledColor :
               root.pressed ? numPadFontDisabledColor :
               root.highlight ? numPadFontActiveColor : numPadNormalFontColor
        font.family: parent.buttonTextFontFamilyName
        font.pixelSize: Settings.local.notification.pincodePrompt.numPadFontSize
        font.weight: Font.Normal
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        width: root.width
        elide: Text.ElideRight
    }
}
