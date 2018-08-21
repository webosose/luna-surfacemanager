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

#include "webosseat.h"
#include "weboscorecompositor.h"
#include "webosinputdevice.h"

#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <QDebug>
#include <QWindow>
#include <QScreen>
#include <QWaylandCompositor>
#include <QString>
#include <QtCompositor/private/qwlinputdevice_p.h>

#define WEBOSINPUTMANAGER_VERSION 1
#define WEBOSSEAT_VERSION 1

/* Native function pointer to get device information */
static void (*getDeviceInfoFunc)(uint32_t devId, QString &devName,
                                 uint32_t *designator, uint32_t *capability);

/* Native function pointer to set keyboard grabbed status */
static void (*setGrabStatusFunc)(uint32_t devId, bool grabbed);

/* Native function to set cursor visibility */
static void (*setCursorVisibilityFunc)(QScreen* screen, bool visibility);

WebOSInputManager::WebOSInputManager(WebOSCoreCompositor* compositor)
    : QtWaylandServer::wl_webos_input_manager(compositor->waylandDisplay(), WEBOSINPUTMANAGER_VERSION)
    , m_compositor(compositor)
    , m_nativeInterface(QGuiApplication::platformNativeInterface())
{
    connect(m_compositor, SIGNAL(cursorVisibleChanged()), this, SLOT(advertiseCursorVisibility()));
    getDeviceInfoFunc = (void(*)(uint32_t, QString&, uint32_t*, uint32_t*))
                        m_nativeInterface->nativeResourceForScreen("getDeviceInfoFunc",
                        (static_cast<QWaylandCompositor*>(m_compositor))->window()->screen());
    setGrabStatusFunc = (void(*)(uint32_t, bool))
                        m_nativeInterface->nativeResourceForScreen("setGrabStatusFunc",
                        (static_cast<QWaylandCompositor*>(m_compositor))->window()->screen());
    setCursorVisibilityFunc = (void(*)(QScreen*, bool))
                              m_nativeInterface->nativeResourceForScreen("setCursorVisibilityFunc",
                              (static_cast<QWaylandCompositor*>(m_compositor))->window()->screen());
}

WebOSInputDevice* WebOSInputManager::findWebOSInputDevice(QtWaylandServer::wl_seat *seat)
{
    QtWaylandServer::wl_seat *seatDefault =
        static_cast<QtWaylandServer::wl_seat *>(m_compositor->defaultInputDevice()->handle());

    //Default Input device doesn't have WebOSInputDevice
    if (seatDefault == seat)
        return NULL;

    QWaylandInputDevice *findInputDev = NULL;
    QList<QWaylandInputDevice *> devices = m_compositor->inputDevices();
    foreach (QWaylandInputDevice *device, devices)
    {
        QtWayland::InputDevice *d = device->handle();
        QtWaylandServer::wl_seat *comparedSeat = static_cast<QtWaylandServer::wl_seat *>(d);
        if (comparedSeat == seat) {
            findInputDev = device;
            return static_cast<WebOSInputDevice*>(device);
        }
    }

    qWarning() << "SHOULD NOT HAPPEN! All input devices except default MUST HAVE WebOSInputDevice!";
    Q_ASSERT(seatDefault == seat || findInputDev);
    return NULL;
}

void WebOSInputManager::getDeviceInfo(int deviceId,  QString &deviceName,
                                    uint32_t *designator, uint32_t *capability)
{
    if (getDeviceInfoFunc)
        getDeviceInfoFunc(deviceId, deviceName, designator, capability);
}

void WebOSInputManager::setGrabStatus(int deviceId, bool grabbed)
{
    if (setGrabStatusFunc)
        setGrabStatusFunc(deviceId, grabbed);
}

void WebOSInputManager::webos_input_manager_set_cursor_visibility(Resource *resource, uint32_t visibility)
{
    Q_UNUSED(resource);

    if (m_compositor->cursorVisible() == visibility)
        return;

    if (setCursorVisibilityFunc)
        setCursorVisibilityFunc(static_cast<QWaylandCompositor*>(m_compositor)->window()->screen(), visibility);
}

void WebOSInputManager::webos_input_manager_get_webos_seat(Resource *resource,
                                                           uint32_t id,
                                                           struct ::wl_resource *seat)
{
    WebOSInputManager *im = static_cast<WebOSInputManager*>(resource->webos_input_manager_object);

    WebOSInputDevice *webosInputDev =
        findWebOSInputDevice(static_cast<QtWaylandServer::wl_seat::Resource *>(seat->data)->seat_object);

    // NOTE: Will be freed from WebOSSeat::webos_seat_destroy_resource()
    new WebOSSeat(im, resource->client(), id, webosInputDev);
}

void WebOSInputManager::webos_input_manager_bind_resource(Resource *resource)
{
    if (!(m_cursorVisibleClient.contains(resource)))
        m_cursorVisibleClient.append(resource);

    /* Cause all pointer-devices' pointer are mapped to one hw cursor right now
       we can consider all cursor's visibility is changed. */
    send_cursor_visibility(resource->handle, m_compositor->cursorVisible()?1:0, NULL);
}

void WebOSInputManager::webos_input_manager_destroy_resource(Resource *resource)
{
    m_cursorVisibleClient.removeAll(resource);
}

void WebOSInputManager::advertiseCursorVisibility()
{
    foreach(Resource* r, m_cursorVisibleClient) {
        /* Cause all pointer-devices' pointer are mapped to one hw cursor right now
        we can consider all cursor is hidden. */
        send_cursor_visibility(r->handle, m_compositor->cursorVisible()?1:0, NULL);
    }
}

WebOSSeat::WebOSSeat(WebOSInputManager *mgr, struct wl_client *client, uint32_t id,
                     WebOSInputDevice* webosInputDev)
    : QtWaylandServer::wl_webos_seat(client, id, WEBOSSEAT_VERSION)
    , m_inputManager(mgr)
    , m_webosInputDev(webosInputDev)
{
    connect(m_webosInputDev, SIGNAL(deviceIdChanged()), this, SLOT(updateDeviceInfo()));
    updateDeviceInfo();
}

void WebOSSeat::updateDeviceInfo()
{
    int deviceId = -1;

    if (!m_webosInputDev) {
        //Default Input Device, device id is always 0.
        deviceId = 0;
    } else if (m_webosInputDev->id() > 0) {
        deviceId = m_webosInputDev->id();
    } else {
        qWarning() << "Device has not been bind yet.";
        return;
    }

    m_inputManager->getDeviceInfo(deviceId, m_deviceName, &m_designator, &m_capabilities);
    send_info((uint32_t)deviceId, m_deviceName.toUtf8().constData(), m_designator, m_capabilities);

    qDebug() << "Sent WebOSSeat info:" << m_deviceName << m_designator << m_capabilities;
}

void WebOSSeat::webos_seat_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void WebOSSeat::webos_seat_get_gyroscope(Resource *resource, uint32_t id)
{
    //TODO:
    Q_UNUSED(resource);
    Q_UNUSED(id);
}

void WebOSSeat::webos_seat_get_accelerometer(Resource *resource, uint32_t id)
{
    //TODO:
    Q_UNUSED(resource);
    Q_UNUSED(id);
}
