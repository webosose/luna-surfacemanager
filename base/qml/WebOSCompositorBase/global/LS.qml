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
                var newList = [];
                // Make a hash table by appId
                for (var i = 0; res.apps[i]; i++) {
                    if (res.apps[i].id)
                        newList[res.apps[i].id] = res.apps[i];
                }
                appInfoList = newList;
            } else if (res.app && res.change !== undefined) {
                // "app" is returned for any change of an individual app
                var newList = appInfoList;
                switch (res.change) {
                    case "added":
                    case "updated":
                        newList[res.app.id] = res.app;
                        break;
                    case "removed":
                        newList[res.app.id] = undefined;
                        break;
                    default:
                        console.warn("Unhandled response for listApps:", applicationList);
                        return;
                }
                appInfoList = newList;
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
