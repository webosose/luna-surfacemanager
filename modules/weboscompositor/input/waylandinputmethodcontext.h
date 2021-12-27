// Copyright (c) 2013-2022 LG Electronics, Inc.
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
#include <QPointer>

#include <wayland-server.h>
#include <wayland-input-method-server-protocol.h>
#include <wayland-text-server-protocol.h>
#include <QtWaylandCompositor/private/qwaylandkeyboard_p.h>
#include "waylandinputpanel.h"
#include "weboskeyboard.h"

class WaylandInputMethod;
class WaylandTextModel;
class WebOSCoreCompositor;

class WEBOS_COMPOSITOR_EXPORT WaylandInputMethodContextDelegate
{
public:
    virtual ~WaylandInputMethodContextDelegate() {}
    virtual void sendSurroundingText(wl_resource *resource, const char *text, uint32_t cursor, uint32_t anchor);
    virtual void sendEnterKeyType(wl_resource *resource, uint32_t enter_key_type);
    virtual void sendContentType(wl_resource *resource, uint32_t hint, uint32_t purpose);
    virtual void sendMaxTextLength(wl_resource *resource, uint32_t length);
    virtual void sendInvokeAction(wl_resource *resource, uint32_t button, uint32_t index);
    virtual void sendCommit(wl_resource *resource);
    virtual void sendReset(wl_resource *resource, uint32_t serial);
};

/*!
 * Implements the compositor side protocol for the input method
 */
class WEBOS_COMPOSITOR_EXPORT WaylandInputMethodContext : public QObject, public KeyboardGrabber
{

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

    WaylandTextModel* textModel() const { return m_textModel; }

    void setDelegate(WaylandInputMethodContextDelegate* delegate);

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

    void showInputPanel();
    void hideInputPanel();

    void focused(QWaylandSurface* surface) override;
    void key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state) override;
    void modifiers(uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) override;
    void updateModifiers() override;

    void updatePanelState(const WaylandInputPanel::InputPanelState state) const;
    void updatePanelRect(const QRect& rect) const;
    void continueTextModelActivation();

signals:
    void contextDestroyed();
    void activated();
    void deactivated();

private:

    void cleanup();
    void releaseGrab();

    void grabKeyboardImpl();
    void releaseGrabImpl();

    QPointer<WaylandInputMethod> m_inputMethod;
    WaylandTextModel* m_textModel;
    wl_resource* m_resource;
    QtWaylandServer::wl_keyboard::Resource *m_grabResource;
    bool m_activated;
    uint32_t m_resourceCount;
    bool m_grabbed;
    WebOSKeyboard *m_grabKeyboard = nullptr;
#ifdef MULTIINPUT_SUPPORT
    WebOSCoreCompositor *m_compositor = nullptr;
#endif
    QScopedPointer<WaylandInputMethodContextDelegate> m_delegate;
};

#endif //WAYLANDINPUTMETHOD_H
