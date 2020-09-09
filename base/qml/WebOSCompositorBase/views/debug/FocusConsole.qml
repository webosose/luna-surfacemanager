// Copyright (c) 2014-2021 LG Electronics, Inc.
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

DebugWindow {
    id: root
    title: "Focus Console"

    mainItem: Rectangle {
        Text {
            id: debugMsg
            anchors.fill: parent
            font.pixelSize: 15
        }
    }

    Connections {
        target: compositorWindow
        function onActiveFocusItemChanged() {
            console.log("ACTIVE FOCUS", compositorWindow.activeFocusItem);

            var items = [];
            var cur = compositorWindow.activeFocusItem;
            do {
                items.push(cur);
                cur = cur.parent;
            } while (cur);

            items.reverse();
            var msg = "";
            for (var i = 0; i < items.length; i++)
                msg += "[" + i + "] " + items[i] + "\n";
            debugMsg.text = msg;
        }
    }
}
