// Copyright (c) 2018-2020 LG Electronics, Inc.
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

#ifndef WEBOSTABLET_H
#define WEBOSTABLET_H

#include <QObject>
#include <QTabletEvent>

#include <QtWaylandCompositor/private/qwayland-server-wayland.h>
#include <WebOSCoreCompositor/private/qwayland-server-webos-tablet.h>

class WebOSCoreCompositor;
class QWaylandView;

class WebOSTablet : public QObject, public QtWaylandServer::wl_webos_tablet
{
    Q_OBJECT
public:
    WebOSTablet(WebOSCoreCompositor* compositor);
    bool postTabletEvent(QTabletEvent*, QWaylandView*);
    void advertiseApproximation(QTabletEvent*);
};

#endif
