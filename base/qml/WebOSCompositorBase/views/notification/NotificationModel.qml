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

ListModel {
    property bool accept
    property var modelData

    // As each response from the individual types of notifications looks very different,
    // override this to parse the data received, and put it into your model in a correct form
    function parseModelData() {
        console.warn("**** You should override parseModelData() in NotificationModel to handle your data!!");
    }

    function unacceptable() {
        console.warn("**** You should override unacceptable() in NotificationModel to handle your data!!");
    }

    onModelDataChanged: {
        if (accept && modelData)
            parseModelData();
        else
            unacceptable();
    }
}
