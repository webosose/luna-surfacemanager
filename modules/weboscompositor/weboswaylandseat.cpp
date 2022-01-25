// Copyright (c) 2021-2022 LG Electronics, Inc.
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

#include "weboscorecompositor.h"
#include "weboswaylandseat.h"

#include <QtCore/QString>
#include <QtWaylandCompositor/private/qwaylandseat_p.h>

WebOSWaylandSeat::WebOSWaylandSeat(QWaylandCompositor *compositor, CapabilityFlags capabilityFlags)
    : QWaylandSeat(compositor, capabilityFlags)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QString ruleNames{qgetenv("QT_WAYLAND_XKB_RULE_NAMES")};
    if (!ruleNames.isEmpty()) {
        // rules:model:layout:variant:options
        QStringList split = ruleNames.split(":");
        if (split.length() == 5) {
            qInfo() << "Using QT_WAYLAND_XKB_RULE_NAMES for default XKB keymap:" << ruleNames;
            keymap()->setRules(split[0]);
            keymap()->setModel(split[1]);
            keymap()->setLayout(split[2]);
            keymap()->setVariant(split[3]);
            keymap()->setOptions(split[4]);
        } else {
            qWarning("Error to parse QT_WAYLAND_XKB_RULE_NAMES. Default XKB keymap will be used.");
        }
    }
    connect(this, &QWaylandSeat::cursorSurfaceRequested, this, &WebOSWaylandSeat::setCursorSurface);
#endif
}

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void WebOSWaylandSeat::setCursorSurface(QWaylandSurface *surface, int hotspotX, int hotspotY, QWaylandClient *qWlClient)
#else
void WebOSWaylandSeat::setCursorSurface(QWaylandSurface *surface, int hotspotX, int hotspotY, wl_client *client)
#endif
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    setCursorSurfaceInternal(surface, hotspotX, hotspotY, qWlClient->client());
#else
    setCursorSurfaceInternal(surface, hotspotX, hotspotY, client);
#endif
}

void WebOSWaylandSeat::setCursorSurfaceInternal(QWaylandSurface *surface, int hotspotX, int hotspotY, wl_client *client)
{
    WebOSCoreCompositor *webos_compositor = static_cast<WebOSCoreCompositor *>(compositor());

    if (!webos_compositor) {
        qWarning() << "could not call setCursorSurface() as compositor() returns null";
        return;
    }

    webos_compositor->setCursorSurface(surface, hotspotX, hotspotY, client);
}
