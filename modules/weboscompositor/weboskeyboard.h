// Copyright (c) 2019-2020 LG Electronics, Inc.
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
#ifndef __WEBOS_KEYBOARD_H__
#define __WEBOS_KEYBOARD_H__

#include <QWaylandKeyboard>
#include <QtWaylandCompositor/private/qwaylandkeyboard_p.h>

class WebOSKeyboard;
class QWaylandSurface;

struct KeyboardGrabber
{
    virtual void focused(QWaylandSurface* surface) = 0;
    virtual void key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state) = 0;
    virtual void modifiers(uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) = 0;
    virtual void updateModifiers() = 0;

    QWaylandKeyboard *m_keyboardPublic = nullptr;
    QWaylandKeyboardPrivate *m_keyboard = nullptr;
};


class WebOSKeyboard: public QWaylandKeyboard
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandKeyboard)
public:
    WebOSKeyboard(QWaylandSeat *seat);

    void setFocus(QWaylandSurface *surface) override;
    void sendKeyModifiers(QWaylandClient *client, uint32_t serial) override;
    void sendKeyPressEvent(uint code, bool repeat) override;
    void sendKeyReleaseEvent(uint code, bool repeat) override;
    void addClient(QWaylandClient *client, uint32_t id, uint32_t version) override;


    void startGrab(KeyboardGrabber *grab);
    void endGrab();
    KeyboardGrabber *currentGrab() const;

private:
    void sendKeyEvent(uint code, uint32_t state, bool repeat);
    void pendingFocusDestroyed(void *data);

private:
    KeyboardGrabber* m_grab = nullptr;
    QtWaylandServer::wl_keyboard::Resource* m_grabResource = nullptr;

    QWaylandSurface *m_pendingFocus = nullptr;
    QWaylandDestroyListener m_pendingFocusDestroyListener;

    uint32_t modsDepressed = 0;
    uint32_t modsLatched = 0;
    uint32_t modsLocked = 0;
    uint32_t group = 0;
};

#endif//__WEBOS_KEYBOARD_H__
