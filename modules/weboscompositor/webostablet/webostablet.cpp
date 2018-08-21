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

#include "webostablet.h"
#include "weboscorecompositor.h"

#include <QGuiApplication>
#include <QDebug>
#include <QWaylandCompositor>
#include <QString>

#include <QtWaylandCompositor/qwaylandsurface.h>
#include <QtWaylandCompositor/qwaylandview.h>

#define WEBOSTABLET_VERSION 1

WebOSTablet::WebOSTablet(WebOSCoreCompositor* compositor)
    : QtWaylandServer::wl_webos_tablet(compositor->display(), WEBOSTABLET_VERSION)
{
}

bool WebOSTablet::postTabletEvent(QTabletEvent* event, QWaylandView* view)
{
    wl_client* client = view->surface()->waylandClient();
    Resource* target = resourceMap().contains(client) ? resourceMap().value(client) : nullptr;
    if (target) {
        send_tablet_event(target->handle, event->uniqueId(), event->pointerType(), event->buttons(),
                      wl_fixed_from_double(event->globalPosF().x()),
                      wl_fixed_from_double(event->globalPosF().y()),
                      event->xTilt(), event->yTilt(),
                      wl_fixed_from_double(event->pressure()),
                      wl_fixed_from_double(event->rotation()));
        return true;
    }
    return false;
}

void WebOSTablet::advertiseApproximation(QTabletEvent* event)
{
    foreach (const Resource* res, resourceMap().values())
        send_tablet_event(res->handle, event->uniqueId(), event->pointerType(), 0, 0, 0, 0, 0, 0, 0);
}
