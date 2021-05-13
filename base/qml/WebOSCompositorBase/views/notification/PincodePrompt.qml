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
import WebOS.Global 1.0
import WebOSCompositorBase 1.0

FocusScope {
    id: root

    property string title
    property string parental_title: qsTr("ENTER PIN CODE") + Settings.l10n.tr
    property string retry_title: qsTr("INCORRECT PIN CODE ENTERED") + Settings.l10n.tr
    property string setmatch_title: qsTr("ENTER PIN CODE") + Settings.l10n.tr
    property string setnewpin_title: qsTr("SET PIN CODE") + Settings.l10n.tr
    property string notpermitted_title: qsTr("INVALID PIN") + Settings.l10n.tr
    property string verify_title: qsTr("CONFIRM PIN CODE") + Settings.l10n.tr
    property string retry_verify_title: qsTr("CONFIRM PIN CODE") + Settings.l10n.tr
    property string backButtonText: qsTr("DELETE") + Settings.l10n.tr
    property string enterButtonText: qsTr("OK") + Settings.l10n.tr
    property string password

    property var retry
    property bool partialRTL: (Qt.locale() === "fa-IR") ? true : false

    width: parent.width
    height: Settings.local.notification.pincodePrompt.height

    signal postPassword()
    signal postExitSignal()

    Rectangle {
        id: pincodePromptBackground
        anchors.fill: parent
        color: Settings.local.notification.pincodePrompt.backgroundColor
    }

    Item {
        width: parent.width
        height: parent.height
        anchors.centerIn: parent

        Text {
            id: pincodePromptText
            objectName: "pincodePromptText"
            text: root.title
            color: Settings.local.notification.pincodePrompt.titleColor
            height: Settings.local.notification.pincodePrompt.titleTextHeight

            anchors.top: parent.top
            anchors.topMargin: Settings.local.notification.pincodePrompt.titleTextTopMargin
            anchors.left: parent.left
            anchors.leftMargin: Settings.local.notification.pincodePrompt.titleTextLeftMargin

            font.family: Settings.local.notification.pincodePrompt.titleFontFamilyName
            font.pixelSize: Settings.local.notification.pincodePrompt.titleFontSize
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.Wrap
            elide: Text.ElideRight

            maximumLineCount: Settings.local.notification.pincodePrompt.textMaxLineCount
        }

        Row {
            id: inputBoxRow
            LayoutMirroring.enabled: Settings.l10n.isRTL
            LayoutMirroring.childrenInherit: root.partialRTL

            anchors.left: parent.left
            anchors.leftMargin: (root.partialRTL) ? Settings.local.notification.pincodePrompt.inputBoxRowRightMargin
                                                  : Settings.local.notification.pincodePrompt.inputBoxRowLeftMargin
            anchors.top: pincodePromptText.bottom
            anchors.topMargin: Settings.local.notification.pincodePrompt.inputBoxRowTopMargin
            spacing: Settings.local.notification.pincodePrompt.inputBoxRowSpacing

            Repeater {
                id: repeaterPasswordBox
                model: Settings.local.notification.pincodePrompt.numOfPWPad
                PincodePromptPasswordButton {
                    objectName: "pincodePromptPasswordButton"
                    current: index === inputFieldIndex
                }
            }
        }

        FocusScope {
            id: buttonsScope
            anchors.fill: parent
            focus: true

            CloseButton {
                id: exitButton
                objectName: "pinExitButton"

                anchors.top: parent.top
                anchors.right: parent.right

                KeyNavigation.down: numPadButtonConfirm.enabled ? numPadButtonConfirm :
                                    numPadButtonBack.enabled ? numPadButtonBack :
                                    buttonRow.lastFocusItem

                onClicked: exitClicked();
            }

            Row {
                id: buttonRow
                property PincodePromptNumButton lastNumItem
                property PincodePromptNumButton lastFocusItem

                LayoutMirroring.enabled: Settings.l10n.isRTL
                LayoutMirroring.childrenInherit: root.partialRTL

                anchors.left: parent.left
                anchors.leftMargin: (root.partialRTL) ? Settings.local.notification.pincodePrompt.buttonRowRightMargin
                                                      : Settings.local.notification.pincodePrompt.buttonRowLeftMargin
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Settings.local.notification.pincodePrompt.buttonRowBottomMargin
                spacing: Settings.local.notification.pincodePrompt.buttonRowSpacing

                Repeater {
                    id: repeaterNumpad
                    model: Settings.local.notification.pincodePrompt.numOfPad
                    PincodePromptNumButton {
                        id: numpadKey
                        objectName: "pinNumpadKey"
                        text: (index == (Settings.local.notification.pincodePrompt.numOfPad - 1)) ? 0 : (index + 1)
                        KeyNavigation.left: repeaterNumpad.itemAt(index - 1)
                        KeyNavigation.right: index < Settings.local.notification.pincodePrompt.numOfPad - 1 ? repeaterNumpad.itemAt(index+1) : numPadButtonBack
                        KeyNavigation.up: exitButton
                        focus: index == 0
                        enabled: inputFieldIndex < Settings.local.notification.pincodePrompt.numOfPWPad

                        Component.onCompleted: {
                            if (index == Settings.local.notification.pincodePrompt.numOfPad - 1) {
                                buttonRow.lastNumItem = numpadKey;
                                buttonRow.lastFocusItem = numpadKey;
                            }
                        }

                        onActiveFocusChanged: {
                            if (activeFocus)
                                buttonRow.lastFocusItem = numpadKey;
                        }
                    }
                }
            }

            PincodePromptNumButton {
                id: numPadButtonBack
                objectName: "pinBackButton"
                LayoutMirroring.enabled: Settings.l10n.isRTL
                LayoutMirroring.childrenInherit: root.partialRTL

                text: backButtonText
                width: Settings.local.notification.pincodePrompt.numPadBackButtonWidth

                anchors.left: buttonRow.right
                anchors.leftMargin: Settings.local.notification.pincodePrompt.numPadButtonBackLeftMargin
                anchors.bottom: buttonRow.bottom

                KeyNavigation.left: buttonRow.lastNumItem
                KeyNavigation.right: numPadButtonConfirm
                KeyNavigation.up: exitButton
                enabled: inputFieldIndex > 0

                onEnabledChanged: {
                    if (!enabled && buttonRow.lastNumItem)
                        buttonRow.lastNumItem.focus = true;
                }
            }

            PincodePromptNumButton {
                id: numPadButtonConfirm
                objectName: "pinConfirmButton"
                LayoutMirroring.enabled: Settings.l10n.isRTL
                LayoutMirroring.childrenInherit: root.partialRTL

                text: enterButtonText
                width: Settings.local.notification.pincodePrompt.numPadButtonConfirmWidth

                anchors.left: numPadButtonBack.right
                anchors.leftMargin: Settings.local.notification.pincodePrompt.numPadButtonConfirmLeftMargin
                anchors.bottom: buttonRow.bottom

                KeyNavigation.left: numPadButtonBack
                KeyNavigation.up: exitButton
                enabled: inputFieldIndex > Settings.local.notification.pincodePrompt.numOfPWPad - 1

                onEnabledChanged: {
                    if (enabled)
                        numPadButtonConfirm.focus = true;
                    else
                        KeyNavigation.left.focus = true;
                }
            }
        }
    }

    Component.onCompleted: {
        for (var i = 0; i < Settings.local.notification.pincodePrompt.numOfPad; ++i)
            repeaterNumpad.itemAt(i).clicked.connect(numpadClicked)
        numPadButtonBack.clicked.connect(backClicked);
        numPadButtonConfirm.clicked.connect(confirmClicked);
    }

    property int inputFieldIndex: 0

    function getCurrentPasswordBox() {
        if (inputFieldIndex >= 0 && inputFieldIndex <= Settings.local.notification.pincodePrompt.numOfPWPad - 1)
            return repeaterPasswordBox.itemAt(inputFieldIndex);
        else
            return null;
    }

    function numpadClicked(text) {
        var currentPasswordBox;

        currentPasswordBox = getCurrentPasswordBox();
        if (currentPasswordBox === null)
            return false;

        currentPasswordBox.text = text;
        ++inputFieldIndex;
        return true;
    }

    function backClicked() {
        var currentPasswordBox;

        if (inputFieldIndex <= 0) {
            inputFieldIndex = 0;
            exitClicked();
            return;
        }

        inputFieldIndex--;
        currentPasswordBox = getCurrentPasswordBox();
        currentPasswordBox.text = "";
    }

    function confirmClicked() {
        for (var i = 0; i < Settings.local.notification.pincodePrompt.numOfPWPad; ++i)
            password += repeaterPasswordBox.itemAt(i).text;
        postPassword();
    }

    function exitClicked() {
        postExitSignal();
    }

    function getPromptTitle(retry, promptType, rev_title) {
        var title = parental_title; // default

        if (rev_title !== undefined && !retry
             && (promptType === "parental" || promptType === "set_match"))
            return rev_title;

        if (promptType === "parental")
            title = retry ? retry_title : parental_title;
        else if (promptType === "set_match")
            title = retry ? retry_title : setmatch_title;
        else if (promptType === "set_newpin")
            title = retry ? notpermitted_title : setnewpin_title;
        else if (promptType === "set_verify")
            title = retry ? retry_verify_title : verify_title;

        return title;
    }

    Keys.onReleased: (event) => {
        switch(event.key) {
        case Qt.Key_0:
            numpadClicked("0");
            break;
        case Qt.Key_1:
            numpadClicked("1");
            break;
        case Qt.Key_2:
            numpadClicked("2");
            break;
        case Qt.Key_3:
            numpadClicked("3");
            break;
        case Qt.Key_4:
            numpadClicked("4");
            break;
        case Qt.Key_5:
            numpadClicked("5");
            break;
        case Qt.Key_6:
            numpadClicked("6");
            break;
        case Qt.Key_7:
            numpadClicked("7");
            break;
        case Qt.Key_8:
            numpadClicked("8");
            break;
        case Qt.Key_9:
            numpadClicked("9");
            break;
        case Qt.Key_Backspace:
        case WebOS.Key_webOS_Back:
            backClicked();
            break;
        }
    }
}
