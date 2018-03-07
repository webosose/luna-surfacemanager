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

#include "webosinputdevice.h"
#include <QtCompositor/private/qwlcompositor_p.h>
#include <QtCompositor/private/qwlinputdevice_p.h>
#include <QtCompositor/private/qwlkeyboard_p.h>

WebOSInputDevice::WebOSInputDevice(QWaylandCompositor *compositor)
    : QWaylandInputDevice(compositor)
    , m_compositor(compositor)
    , m_deviceId(-1) //No device ID yet, will be set with queryInputDevice in qtwayland
{
    m_compositor->handle()->registerInputDevice(this);

    /* To trigger keyboard_enter after requested get_keyboard by clinet */
    setKeyboardFocus(m_compositor->defaultInputDevice()->keyboardFocus());

    /* If the other input devices have been grabbed, grab this one too */
    if ( m_compositor->defaultInputDevice()->handle()->keyboardDevice() !=
         m_compositor->defaultInputDevice()->handle()->keyboardDevice()->currentGrab()) {
        handle()->keyboardDevice()->startGrab(
                m_compositor->defaultInputDevice()->handle()->keyboardDevice()->currentGrab());
    }

    /* We use multiple keyboard device but use same modifier state */
    updateModifierState(m_compositor->defaultInputDevice());
}

WebOSInputDevice::~WebOSInputDevice()
{
    m_compositor->handle()->removeInputDevice(this);
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
