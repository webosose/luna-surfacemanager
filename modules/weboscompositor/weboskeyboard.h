// Copyright (c) 2019-2024 LG Electronics, Inc.
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
#ifndef WEBOS_KEYBOARD_H //
#define WEBOS_KEYBOARD_H //

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QWaylandKeyboard>

class QWaylandSurface;
class QWaylandDestroyListener;

struct KeyboardGrabber
{
    virtual void focused(QWaylandSurface* surface) = 0;
    virtual void key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state) = 0;
    virtual void modifiers(uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) = 0;
    virtual void updateModifiers() = 0;

    QWaylandKeyboard *m_keyboardPublic = nullptr;
    QWaylandKeyboardPrivate *m_keyboard = nullptr;
};


class WEBOS_COMPOSITOR_EXPORT WebOSKeyboard: public QWaylandKeyboard
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandKeyboard)
public:
    WebOSKeyboard(QWaylandSeat *seat);
    ~WebOSKeyboard() override;

    void setFocus(QWaylandSurface *surface) override;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    void sendKeyPressEvent(uint code) override;
    void sendKeyReleaseEvent(uint code) override;
#else
    void sendKeyPressEvent(uint code, bool repeat) override;
    void sendKeyReleaseEvent(uint code, bool repeat) override;
#endif
    void addClient(QWaylandClient *client, uint32_t id, uint32_t version) override;

    virtual void updateModifierState(uint code, uint32_t state, bool repeat);

    void startGrab(KeyboardGrabber *grab);
    void endGrab();
    KeyboardGrabber *currentGrab() const;

private:
    void sendKeyEvent(uint code, uint32_t state);
    void pendingFocusDestroyed(void *data);

private:
    KeyboardGrabber* m_grab = nullptr;
    QWaylandSurface *m_pendingFocus = nullptr;
    QWaylandDestroyListener* m_pendingFocusDestroyListener = nullptr;

    uint32_t modsDepressed = 0;
    uint32_t modsLatched = 0;
    uint32_t modsLocked = 0;
    uint32_t group = 0;
};

#endif //WEBOS_KEYBOARD_H
