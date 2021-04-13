// Copyright (c) 2021 LG Electronics, Inc.
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

#ifndef WEBOSWAYLANDSEAT_H
#define WEBOSWAYLANDSEAT_H

#include <QtWaylandCompositor/qwaylandseat.h>

class WebOSWaylandSeatPrivate;
class QWaylandCompositor;

class WebOSWaylandSeat : public QWaylandSeat
{
public:
    WebOSWaylandSeat(QWaylandCompositor *compositor, CapabilityFlags capabilityFlags = DefaultCapabilities);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
public slots:
    void setCursorSurface(QWaylandSurface *surface, int hotspotX, int hotspotY, QWaylandClient *client);
#else
    void setCursorSurface(QWaylandSurface *surface, int hotspotX, int hotspotY, wl_client *client) override;
#endif
};

#endif // WEBOSWAYLANDSEAT_H
