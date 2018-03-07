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

Notification {
    id: root

    property var model
    property Component delegate
    property bool enabled: true

    property bool active: windowListView.count > 0

    // Default
    width: 100
    height: 100

    onEnabledChanged: {
        if (!enabled && root.state === "open")
            root.model.clear();
    }

    onFocusChanged: {
        windowListView.focus = focus;
    }

    ListView {
        id: windowListView
        z: 5
        anchors.fill: parent

        model: root.model
        delegate: root.delegate
        spacing: currentItem ? -currentItem.height : 0
        orientation: ListView.Vertical
        interactive: false
        ListView.delayRemove: true

        property int previousCount: 0

        onCountChanged: {
            if (currentIndex > 0)
                currentIndex = 0;
            if (root.enabled && count) {
                if (count > previousCount)
                    root.state = "open";
            } else if (count === 0) {
                root.state = "closed";
            }
            previousCount = count;
        }
    }
}
