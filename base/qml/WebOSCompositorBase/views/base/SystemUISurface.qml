// Copyright (c) 2020 LG Electronics, Inc.
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

PopupSurface {
    id: root

    Loader {
        id: serverSideAddOn
        anchors.top: parent.top
        anchors.left: parent.left
        asynchronous: true
        active: root.source
        z: root.source ? root.source.z + 1 : 1

        onStatusChanged: {
            console.info("SystemUISurface.serverSideAddOn: status changed", serverSideAddOn.source, serverSideAddOn.status, serverSideAddOn.active);
            if (root.source === undefined)
                return;

            switch (serverSideAddOn.status) {
            case Loader.Null:
                root.source.setAddonStatus(SurfaceItem.AddonStatusNull);
                break;
            case Loader.Ready:
                root.source.setAddonStatus(SurfaceItem.AddonStatusLoaded);
                break;
            case Loader.Loading:
                // Do not report for this case, but just wait for another one.
                break;
            case Loader.Error:
                root.source.setAddonStatus(SurfaceItem.AddonStatusError);
                break;
            }
        }

        Connections {
            target: source
            onAddonChanged: {
                console.info("SystemUISurface.serverSideAddOn: addon changed to" + root.source.addon);
                if (source.type != "_WEBOS_WINDOW_TYPE_SYSTEM_UI") {
                    console.warn("SystemUISurface.serverSideAddOn: denied by window type", source.appId, source.type);
                    source.setAddonStatus(SurfaceItem.AddonStatusDenied);
                    return;
                }

                const match = (path) => source.addon.startsWith(path);
                if (source.addon && !Settings.local.addon.directories.some(match)) {
                    console.warn("SystemUISurface.serverSideAddOn: denied by path", source.appId, source.addon);
                    source.setAddonStatus(SurfaceItem.AddonStatusDenied);
                    return;
                }

                console.info("SystemUISurface.serverSideAddOn: accepted", source.addon, source.appId, source.type);
                serverSideAddOn.source = source.addon;
            }
        }
    }
}
