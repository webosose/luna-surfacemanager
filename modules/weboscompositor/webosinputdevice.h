// Copyright (c) 2013-2021 LG Electronics, Inc.
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

#ifndef WEBOSINPUTDEVICE_H
#define WEBOSINPUTDEVICE_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QInputEvent>
#include <QWaylandSeat>
#include <QWaylandCompositor>
#include "weboswaylandseat.h"

class WEBOS_COMPOSITOR_EXPORT WebOSInputDevice : public WebOSWaylandSeat
{
    Q_OBJECT
public:
    WebOSInputDevice(QWaylandCompositor *compositor);
    WebOSInputDevice(QWaylandCompositor *compositor, CapabilityFlags caps);
    ~WebOSInputDevice();

    void setDeviceId(QInputEvent *event);
    bool isOwner(QInputEvent *event) const override;
    static int getDeviceId(QInputEvent *event);
    int  id() { return m_deviceId; }

signals:
    void deviceIdChanged();

private:
    QWaylandCompositor *m_compositor;
    int m_deviceId;
};

#endif
