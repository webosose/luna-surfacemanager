// Copyright (c) 2019 LG Electronics, Inc.
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

MouseArea {
    id: root
    hoverEnabled: false

    property int threshold: 100

    // Trigger direction
    // vertical: false, reverse: false => right
    // vertical: false, reverse: true  => left
    // vertical: true,  reverse: false => downward
    // vertical: true,  reverse: true  => upward
    property bool vertical: false
    property bool reverse: false

    // internal
    property bool active: true
    property real hp
    property real vp
    property real oldHp
    property real oldVp

    signal triggered

    onPressed: {
        root.active = true;
    }

    onReleased: {
        root.active = false;
        root.hp = !vertical && reverse ? width : 0;
        root.vp = vertical && reverse ? height : 0;
        root.oldHp = root.hp;
        root.oldVp = root.vp;
    }

    onPositionChanged: {
        if (root.active) {
            hp = mouse.x;
            vp = mouse.y;
        }
    }

    // Trigger if the event is the first one that exceeds the threshold.
    // Depending on vertical

    onHpChanged: {
        if (root.active && !root.vertical) {
            if ((root.reverse && root.oldHp >= -root.threshold && root.hp < -root.threshold) ||
                (!root.reverse && root.oldHp <= root.threshold && root.hp > root.threshold))
                root.triggered();
        }
        root.oldHp = root.hp;
    }

    onVpChanged: {
        if (root.active && root.vertical) {
            if ((root.reverse && root.oldVp >= -root.threshold && root.vp < -root.threshold) ||
                (!root.reverse && root.oldVp <= root.threshold && root.vp > root.threshold))
                root.triggered();
        }
        root.oldVp = root.vp;
    }
}
