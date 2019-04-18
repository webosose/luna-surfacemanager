// Copyright (c) 2013-2020 LG Electronics, Inc.
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

#include "webosinputdevice.h"
#include "weboscorecompositor.h"
#include "weboskeyboard.h"

#include <QtWaylandCompositor/private/qwaylandcompositor_p.h>
#include <QtWaylandCompositor/private/qwaylandseat_p.h>
#include <QtWaylandCompositor/private/qwaylandkeyboard_p.h>

WebOSInputDevice::WebOSInputDevice(QWaylandCompositor *compositor)
    : QWaylandSeat(compositor, DefaultCapabilities)
    , m_compositor(compositor)
    , m_deviceId(-1) //No device ID yet, will be set with queryInputDevice in qtwayland
{
    WebOSCoreCompositor *wcompositor = static_cast<WebOSCoreCompositor *>(m_compositor);
    WebOSKeyboard *wkeyboard = static_cast<WebOSKeyboard *>(wcompositor->defaultSeat()->keyboard());

    wcompositor->registerSeat(this);

    /* To trigger keyboard_enter after requested get_keyboard by clinet */
    setKeyboardFocus(wcompositor->defaultSeat()->keyboardFocus());

    /* If the other input devices have been grabbed, grab this one too */
    if (wkeyboard->currentGrab() && wkeyboard != wkeyboard->currentGrab()->m_keyboardPublic) {
        WebOSKeyboard *this_wkeyboard = static_cast<WebOSKeyboard *>(keyboard());
        this_wkeyboard->startGrab(wkeyboard->currentGrab());
    }
    /* We use multiple keyboard device but use same modifier state */
    updateModifierState(wcompositor->defaultSeat());
}

// This constuctor is for window-dedicated device, not multi-input support
WebOSInputDevice::WebOSInputDevice(QWaylandCompositor *compositor, CapabilityFlags caps)
    : QWaylandSeat(compositor, caps)
    , m_compositor(compositor)
    , m_deviceId(-1) // should not be set, distinguished from multi input deivices.
{
    WebOSCoreCompositor *wcompositor = static_cast<WebOSCoreCompositor *>(m_compositor);
    wcompositor->registerSeat(this);

    // Nothing to follow up, the device is created at the initial stage.
}

WebOSInputDevice::~WebOSInputDevice()
{
    WebOSCoreCompositor *wcompositor = static_cast<WebOSCoreCompositor *>(m_compositor);
    wcompositor->unregisterSeat(this);
}

void WebOSInputDevice::setDeviceId(QInputEvent *event)
{
    int deviceId = getDeviceId(event);

    if (m_deviceId != deviceId ) {
        m_deviceId = deviceId;
        emit deviceIdChanged();
    }
}

int WebOSInputDevice::getDeviceId(QInputEvent *event)
{
    Qt::KeyboardModifiers mods = event->modifiers();
    int deviceId = mods & ~Qt::KeyboardModifierMask;

    return deviceId;
}

bool WebOSInputDevice::isOwner(QInputEvent *event)
{
    int deviceId = getDeviceId(event);

    if (deviceId == m_deviceId) {
        return true;
    }

    return false;
}
