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

QtObject {
    property var list: [
        // Handler should return subscribed value.
        {
            service: "com.webos.settingsservice",
            method: "getSystemSettings",
            handler: function (response) {
                if (response.settings !== undefined) {
                    for (var keys in response.settings)
                        return response.settings[keys];
                }
            }
        },
        {
            service: "com.webos.service.config",
            method: "getConfigs",
            handler: function (response) {
                if (response.configs !== undefined) {
                    for (var keys in response.configs)
                        return response.configs[keys];
                }
            }
        }
    ]
}
