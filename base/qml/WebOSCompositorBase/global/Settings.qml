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
import WebOS.Global 1.0
import WebOSCompositorBase 1.0

import "../../WebOSCompositor"

Item {
    id: root
    objectName: "Settings"

    // NOTE: Define properties readonly as they're shared globally

    readonly property string appId: LS.appId
    readonly property alias local: localSettings.settings
    readonly property alias system: systemSettings.settings
    readonly property alias l10n: systemSettings.l10n

    function subscribe(type, method, key) {
        return systemSettings.subscribeOnDemand(type, method, key);
    }

    DefaultSettings {
        id: defaultSettingsData
    }

    LocalSettings {
        id: localSettings

        rootElement: root.appId
        defaultSettings: defaultSettingsData.settings
        files: [
            Qt.resolvedUrl("../settings.json"),
            Qt.resolvedUrl("../../WebOSCompositor/settings.json")
        ]

        Connections {
            target: compositor
            onReloadConfig: {
                localSettings.reloadAllFiles();
                console.info("All settings are reloaded.");
            }
        }
    }

    SystemSettings {
        id: systemSettings
        appId: root.appId
        l10nFileNameBase: "luna-surfacemanager"
        l10nDirName: WebOS.localizationDir + "/" + l10nFileNameBase
        l10nPluginImports: root.local.localization.imports
    }

    Component.onCompleted: console.info("Constructed a singleton type:", root);
}
