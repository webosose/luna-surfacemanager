// Copyright (c) 2018-2019 LG Electronics, Inc.
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

Rectangle {
    id: debugExportedItem;
    color: "red";
    anchors.fill: parent;
    Text {
        anchors.fill: parent;
        text: "EXPORTED REGION! " + x + ":" + y + ":" + width + ":" + height
            + "\nParent: " + debugExportedItem.parent + ":"
            + debugExportedItem.parent.x + ":" + debugExportedItem.parent.y
            + "\nParent2:" + debugExportedItem.parent.parent
            + ":" + debugExportedItem.parent.parent.x + ":" + debugExportedItem.parent.parent.y;
        font.pointSize: 24
    }
}
