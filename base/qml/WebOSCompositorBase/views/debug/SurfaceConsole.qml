// Copyright (c) 2014-2018 LG Electronics, Inc.
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
import Eos.Items 0.1 as Eos

DebugWindow {
    id: root
    title: "Surface Console: " + (root.model.objectName || root.model)
    statusText: "count: " + view.count
    property var model: compositor.surfaceModel

    mainItem: Rectangle {
        ListView {
            id: view
            model: root.model
            delegate: Rectangle {
                id: delegate
                property Item surface: model.surfaceItem
                width: parent.width
                height: data.height
                border.color: "black"
                border.width: 1

                Eos.FastParallelogram {
                    anchors.right: parent.right
                    angle: 0
                    height: parent.height
                    width: (height / 9) * 16
                    color: "darkgray"
                    sourceItem: delegate.surface
                }

                Column {
                    id: data
                    width: parent.width
                    anchors.margins: 4
                    Text { text: surface ? "["+ model.index+"]" + surface.appId : ""; font.pixelSize: 15 }
                    Text { text: surface ? "fullscreen: " + surface.fullscreen : ""; font.pixelSize: 15 }
                    Text { text: surface ? "tick: "+surface.lastFullscreenTick : ""; font.pixelSize: 15 }
                    Text { text: surface ? "snap: "+surface.cardSnapShotFilePath : ""; font.pixelSize: 15 }
                }
            }
        }
    }
}
