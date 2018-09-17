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

#ifndef WAYLANDINPUTMETHODCONTEXT_H
#define WAYLANDINPUTMETHODCONTEXT_H

#include <QObject>
#include <QRect>

#include <wayland-server.h>
#include <wayland-input-method-server-protocol.h>
#include <wayland-text-server-protocol.h>
#include <QtCompositor/private/qwlkeyboard_p.h>
#include "waylandinputpanel.h"

class WaylandInputMethod;
class WaylandTextModel;

/*!
 * Implements the compositor side protocol for the input method
 */
class WaylandInputMethodContext : public QObject, public QtWayland::KeyboardGrabber {

    Q_OBJECT

public:
    WaylandInputMethodContext(WaylandInputMethod* inputMethod, WaylandTextModel* model);
    ~WaylandInputMethodContext();

    static void destroy(struct wl_client *client, struct wl_resource *resource);
    static void commitString(struct wl_client *client, struct wl_resource *resource, uint32_t serial, const char *text);
    static void preEditString(struct wl_client *client, struct wl_resource *resource, uint32_t serial, const char *text, const char* commit);
    static void preEditStyling(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t index, uint32_t length, uint32_t style);
    static void preEditCursor(struct wl_client *client, struct wl_resource *resource, uint32_t serial, int32_t index);


    static void deleteSurroundingText(struct wl_client *client, struct wl_resource *resource, uint32_t serial, int32_t index, uint32_t length);
    static void cursorPosition(struct wl_client *client, struct wl_resource *resource, uint32_t serial, int32_t index, int32_t anchor);
    static void modifiersMap(struct wl_client *client, struct wl_resource *resource, struct wl_array *map);
    static void keySym(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers);
    static void grabKeyboard(struct wl_client *client, struct wl_resource *resource, uint32_t keyboard);
    static void key(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    static void modifiers(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

    static const struct input_method_context_interface inputMethodContextImplementation;

    static void destroyInputMethodContext(struct wl_resource* resource);

    wl_resource* handle() { return m_resource; }

    void deactivate();
    void maybeDestroy();

public slots:
    void activateTextModel();
    void deactivateTextModel();
    void destroyTextModel();
    void updateContentType(uint32_t hint, uint32_t purpose);
    void updateEnterKeyType(uint32_t enter_key_type);
    void updateSurroundingText(const QString& text, uint32_t cursor, uint32_t anchor);
    void resetContext(uint32_t serial);
    void commit();
    void invokeAction(uint32_t button, uint32_t index);
    void maxTextLengthTextModel(uint32_t length);
    void platformDataModel(const QString& text);


    void focused(QtWayland::Surface* surface) Q_DECL_OVERRIDE;
    void key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state) Q_DECL_OVERRIDE;
    void modifiers(uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) Q_DECL_OVERRIDE;

    void updatePanelState(const WaylandInputPanel::InputPanelState state) const;
    void updatePanelRect(const QRect& rect) const;
    void continueTextModelActivation();

Q_SIGNALS:
    void contextDestroyed();
    void activated();
    void deactivated();

private:

    void cleanup();
    void releaseGrab();

    void grabKeyboardImpl();
    void releaseGrabImpl();

    WaylandInputMethod* m_inputMethod;
    WaylandTextModel* m_textModel;
    wl_resource* m_resource;
    QtWayland::Keyboard::Resource* m_grabResource;
    bool m_activated;
    uint32_t m_resourceCount;
    bool m_grabbed;
};

#endif //WAYLANDINPUTMETHOD_H
