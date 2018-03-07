// Copyright (c) 2018 LG Electronics, Inc.
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

FocusScope {
    id: root
    visible: false

    signal opening
    signal opened
    signal closing
    signal closed
    signal contentChanged

    property bool isOpen: state == "open"
    property bool access: true
    property SequentialAnimation openAnimation: defaultAnimation
    property SequentialAnimation closeAnimation: defaultAnimation
    property bool isTransitioning: openTransition.running || closeTransition.running

    function openView() {
        if (root.state != "open") {
            console.log("BaseView: start to open", root);
            root.state = "open";
        }
    }

    function closeView() {
        if (root.state != "closed") {
            console.log("BaseView: start to close", root);
            root.state = "closed";
        }
    }

    SequentialAnimation {
        id: defaultAnimation
        PauseAnimation {
            duration: 0
        }
    }

    onContentChanged: {
        console.log("BaseView: content changed in", root);
        compositor.updateCursorFocus();
    }

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
            id: openTransition
            to: "open"
            animations: [ openAnimation ]
            onRunningChanged: {
                if (running) {
                    root.visible = true;
                    console.log("BaseView: emit opening", root);
                    root.opening();
                } else {
                    console.log("BaseView: emit opened", root);
                    root.opened();
                    compositor.updateCursorFocus();
                }
            }
        },
        Transition {
            id: closeTransition
            to: "closed"
            animations: [ closeAnimation ]
            onRunningChanged: {
                if (running) {
                    console.log("BaseView: emit closing", root);
                    root.closing();
                } else {
                    root.visible = false;
                    console.log("BaseView: emit closed", root);
                    root.closed();
                    compositor.updateCursorFocus();
                }
            }
        }
    ]
}
