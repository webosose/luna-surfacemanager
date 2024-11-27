// Copyright (c) 2020-2024 LG Electronics, Inc.
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
import WebOSCoreCompositor 1.0
import WebOSCompositorBase 1.0

Item {
    id: root

    property int clickCount: 0
    property int hitCount: 5
    property real hitRatio: 0.3
    property var optionNames: ["compositorBorder", "surfaceHighlight", "surfaceStack", "touchHighlight"]

    Timer {
        id: clickCountTimer
        interval: 2000

        onTriggered: {
            console.log("DebugPattern: resetting clickCount to 0 from", clickCount);
            clickCount = 0
        }
    }

    Connections {
        target: compositorWindow
        function onDebugTouchUpdated(evt) {
            if (!evt)
                return;

            if (evt.touchPoints.length != 2)
                return;

            var idx_left = getIndexOfLeftPoint(evt.touchPoints, hitRatio);
            if (idx_left == -1)
                return;

            var idx_right = 1 - idx_left;
            var left = evt.touchPoints[idx_left];
            var right = evt.touchPoints[idx_right];

            if (left.state == DebugTouchPoint.TouchPointStationary &&
                right.state == DebugTouchPoint.TouchPointPressed &&
                right.normalizedPos.x > (1 - hitRatio)) {
                if (clickCount == 0)
                    clickCountTimer.start();
                clickCount += 1;
                console.log("DebugTouchPoint: clickCount:", clickCount);
            }

            if (clickCount >= hitCount) {
                clickCountTimer.stop();
                clickCount = 0;

                console.info("DebugTouchPoint: pattern recognized, toggling options", root.optionNames);
                var value = !Settings.local.debug.enable;
                var obj = {debug: {enable: value}};
                for (var i = 0; i < root.optionNames.length; i++) {
                    obj.debug[root.optionNames[i]] = value;
                }
                Settings.updateLocalSettings(obj);
            }
        }
    }

    function getIndexOfLeftPoint(touchPoints, hitRatio) {
        for (var i = 0; i < touchPoints.length; i++) {
            var p = touchPoints[i].normalizedPos;
            if (p.x < hitRatio && p.y < hitRatio)
                return i;
        }
        return -1;
    }
}
