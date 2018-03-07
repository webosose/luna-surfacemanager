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

Popup {
    id: root

    z: 10000

    property int timeout: 0
    property bool timeoutEnabled: true
    property Item background

    onBackgroundChanged: background.parent = root;

    signal timeoutTriggered()

    Timer {
        id: closingTimer
        interval: root.timeout
        running: root.timeout && root.timeoutEnabled
        onTriggered: {
            root.state = "closed";
            root.timeoutTriggered();
        }
    }

    function restartTimer() {
        if (root.timeout > 0) {
             closingTimer.restart();
             closingMasterTimer.stop();
        }
    }

    Timer {
        id: closingMasterTimer
        interval: Settings.local.notification.timeout
        onTriggered: {
            root.state = "closed";
            root.timeoutTriggered();
        }
    }

    function restartMasterTimer() {
         closingMasterTimer.restart();
    }

    function stopMasterTimer() {
         closingMasterTimer.stop();
    }

    state: "closed"
    states: [
        State {
            name: "open"
            PropertyChanges {
                target: root
                opacity: 1.0
            }
        },
        State {
            name: "closed"
            PropertyChanges {
                target: root
                opacity: 0.0
            }
        }
    ]

    transitions: [
        Transition {
            from: "closed"
            to: "open"
            reversible: true
            SequentialAnimation {
                ScriptAction {
                    script: root.close();
                }
                NumberAnimation {
                    target: root
                    properties: "opacity"
                    duration: 300
                }
                ScriptAction {
                    script: root.open();
                }
            }
        }
    ]
}
