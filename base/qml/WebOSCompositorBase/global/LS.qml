// Copyright (c) 2017-2018 LG Electronics, Inc.
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

pragma Singleton

import QtQuick 2.4
import AudioService 1.0
import WebOSServices 1.0

QtObject {
    id: root
    objectName: "LS"

    // NOTE: Define properties readonly as they're shared globally

    readonly property string appId: "com.webos.surfacemanager"

    readonly property Service adhoc: Service {
        appId: root.appId
    }

    readonly property ApplicationManagerService applicationManager: ApplicationManagerService {
        appId: root.appId

        property var appInfoList: ({})

        onConnectedChanged: {
            if (connected) {
                subscribeAppLifeStatus();
                subscribeApplicationList();
            }
        }

        onApplicationListChanged: {
            var res = JSON.parse(applicationList);
            if (res.apps) {
                // Make a hash table by appId
                for (var i = 0; res.apps[i]; i++) {
                    if (res.apps[i].id)
                        appInfoList[res.apps[i].id] = res.apps[i];
                }
            }
            // "app" is returned when the information of an app has been installed/updated/removed.
            if (res.app) {
                if (res.change == "added" || res.change == "updated")
                    appInfoList[res.app.id] = res.app;
                if (res.change == "removed")
                    appInfoList[res.app.id] = undefined;
            }
        }
    }

    readonly property AudioService audioService: AudioService {
        appId: root.appId + ".audio"
    }

    Component.onCompleted: {
        console.info("Constructed a singleton type:", root);
        console.log("Initial volume: " + audioService.volume +
                    ", scenario: " + audioService.scenario +
                    ", muted: " + audioService.muted +
                    ", disabled: " + audioService.disabled);
    }
}
