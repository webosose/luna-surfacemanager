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
import WebOS.Global 1.0

QtObject {
    readonly property url imagePath: Qt.resolvedUrl("resources/images/")
    readonly property var settings: {
        "addon": {
            "directories": [
                "file://" + WebOS.applicationDir + "/"
            ]
        },
        "compositor": {
            "geometryPendingInterval": 2000
        },
        "debug": {
            "enable": false,
            "compositorBorder": false,
            "focusHighlight": false,
            "surfaceHighlight": false,
            "surfaceStack": false,
            "fpsGraphOverlay": false,
            "spinnerRepaint": false,
            "touchOverlay": false,
            "resourceMonitor": false,
            "logConsole": false,
            "logFilter": {
                "debug": true,
                "warning": true,
                "critical": true,
                "fatal": true
            },
            "focusConsole": false,
            "surfaceConsole": false,
            "debugWindow": {
                "x": 100,
                "y": 100,
                "width": 600,
                "height": 500,
                "opacity": 0.8
            }
        },
        "imageResources": {
            "settings": imagePath + "settings.png",
            "spinner": imagePath + "spinner.gif",
            "pinBox": imagePath + "pinprompt_box_n.png",
            "pinBoxFocused": imagePath + "pinprompt_box_f.png",
            "pinBoxDot": imagePath + "pinprompt_box_dot_n.png",
            "pinBoxDotFocused": imagePath + "pinprompt_box_dot_f.png"
        },
        "keyboardView": {
            "height": 324,
            "slideAnimationDuration": 500
        },
        "launcher": {
            "width": 800,
            "opacity": 0.8,
            "backgroundColor": "#383838",
            "defaultMargin": 20,
            "settingsIconSize": 70,
            "slideAnimationDuration": 500,
            "hotspotThickness": 10,
            "hotspotThreshold": 100
        },
        "localization": {
            "imports": []
        },
        "notification": {
            "scrimOpacity": 0.3,
            "timeout": 15000,
            "button": {
                "width": 300,
                "height": 85,
                "backgroundColor": "#ffffff",
                "activatedColor": "#cf0652",
                "fontName": "Miso",
                "fontSize": 52,
                "textColor": "#4b4b4b",
                "textActivatedColor": "#ffffff"
            },
            "closeButton": {
                "size": 60,
                "margins": 10,
                "foregroundColor": "#ffffff",
                "backgroundColor": "#4d4d4d",
                "activatedColor": "#cf0652"
            },
            "alert": {
                "height": 320,
                "buttonMargins": 40,
                "foregroundColor": "#ffffff",
                "backgroundColor": "#4d4d4d",
                "headerFont": "Miso",
                "fontName": "Museo Sans",
                "fontSize": 82,
                "fontWeight": Font.Bold,
                "descriptionFontSize": 26,
                "descriptionLineHeight": 36,
                "descriptionTopMargin": 20,
                "iconSize": 96,
                "iconMargins": 63,
                "textMargins": 40,
                "textIconWidth": 204,
                "textWidth": 940,
                "slideAnimationDuration": 500
            },
            "toast": {
                "width": 760,
                "height": 120,
                "margins": 20,
                "foregroundColor": "#ffffff",
                "backgroundColor": "#4d4d4d",
                "activatedColor": "#cf0652",
                "iconSize": 80,
                "fontName": "Museo Sans",
                "fontSize": 26,
                "fontWeight": Font.Bold,
                "textLineHeight": 40,
                "lineLimitNormal": 2,
                "lineLimitExpandable": 2,
                "timeout": 5000,
                "slideAnimationDuration": 500
            },
            "pincodePrompt": {
                "height": 320,
                "backgroundColor": "#4d4d4d",
                "titleColor": "#d0d0d0",
                "titleFontFamilyName": "Miso",
                "titleFontSize": 73,
                "titleTextHeight": 86,
                "titleTextLeftMargin": 36,
                "titleTextTopMargin": 29,
                "textMaxLineCount": 1,
                "boxSize": 100,
                "buttonRowBottomMargin": 10,
                "buttonRowLeftMargin": 36,
                "buttonRowRightMargin": 80,
                "buttonRowSpacing": 6,
                "inputBoxRowLeftMargin": 36,
                "inputBoxRowRightMargin": 80,
                "inputBoxRowSpacing": 20,
                "inputBoxRowTopMargin": 14,
                "numOfPad": 10,
                "numOfPWPad": 4,
                "numPadBackButtonWidth": 172,
                "numPadButtonBackLeftMargin": 76,
                "numPadButtonConfirmLeftMargin": 10,
                "numPadButtonConfirmWidth": 172,
                "numPadButtonHeight": 60,
                "numPadButtonWidth": 132,
                "numPadActiveColor": "#cf0652",
                "numPadBorderWidth": 5,
                "numPadColor": "#696969",
                "numPadNormalFontColor": "#eeeeee",
                "numPadFontActiveColor": "#ffffff",
                "numPadFontDisabledColor": "#8b8b8b",
                "numPadFontFamilyName": "Museo Sans",
                "numPadFontSize": 28,
                "slideAnimationDuration": 500
            }
        },
        "spinner": {
            "timeout": 20000,
            "scrimOpacity": 0.7,
            "fadeAnimationDuration": 500
        }
    }
}
