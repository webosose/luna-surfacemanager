// Copyright (c) 2013-2018 LG Electronics, Inc.
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

#ifndef WEBOSINPUTMANAGER_H
#define WEBOSINPUTMANAGER_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QObject>
#include <QHash>
#include <QtCompositor/qwaylandinput.h>
#include <QtCompositor/private/qwayland-server-wayland.h>
#include <wayland-server.h>
#include <WebOSCoreCompositor/private/qwayland-server-webos-input-manager.h>

class QWaylandInputDevice;
class WebOSCoreCompositor;
class QPlatformNativeInterface;
class WebOSCompositor;
class WebOSInputDevice;

class WEBOS_COMPOSITOR_EXPORT WebOSInputManager : public QObject, public QtWaylandServer::wl_webos_input_manager {

    Q_OBJECT

public:
    WebOSInputManager(WebOSCoreCompositor* compositor);
    void getDeviceInfo(int deviceId, QString &deviceName, uint32_t *designator, uint32_t *capability);
    void setGrabStatus(int deviceId, bool grabbed);

public slots:
    void advertiseCursorVisibility();

private:
    WebOSCoreCompositor* m_compositor;
    QPlatformNativeInterface *m_nativeInterface;
    WebOSInputDevice* findWebOSInputDevice(QtWaylandServer::wl_seat *seat);
    QList<Resource *> m_cursorVisibleClient;

protected:
    void webos_input_manager_set_cursor_visibility(Resource *resource, uint32_t visibility);
    void webos_input_manager_get_webos_seat(Resource *resource, uint32_t id, struct ::wl_resource *seat);
    void webos_input_manager_bind_resource(Resource *resource);
    void webos_input_manager_destroy_resource(Resource *resource);
};

class WebOSSeat : public QObject, public QtWaylandServer::wl_webos_seat {

    Q_OBJECT

public:
    WebOSSeat(WebOSInputManager *mgr, struct wl_client *client, uint32_t id, WebOSInputDevice *webosInputDev);

public slots:
    void updateDeviceInfo();

protected:
    void webos_seat_get_gyroscope(Resource *resource, uint32_t id);
    void webos_seat_get_accelerometer(Resource *resource, uint32_t id);
    void webos_seat_destroy_resource(Resource *resource);

private:
    WebOSInputManager *m_inputManager;
    WebOSInputDevice *m_webosInputDev;
    QString m_deviceName;
    uint32_t m_designator;
    uint32_t m_capabilities;
};
#endif
