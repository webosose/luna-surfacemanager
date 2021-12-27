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

#ifndef WAYLANDTEXTMODEL_H
#define WAYLANDTEXTMODEL_H

#include <QObject>
#include <QPointer>

#include <wayland-server.h>
#include <wayland-text-server-protocol.h>
#include "waylandinputmethod.h"
#include "waylandinputpanel.h"

class WaylandInputMethodContext;
class WaylandTextModelFactory;
class WebOSSurfaceItem;

class WEBOS_COMPOSITOR_EXPORT WaylandTextModelDelegate
{
public:
    virtual ~WaylandTextModelDelegate() {}
    virtual void commitString(wl_resource* resource, uint32_t serial, const char *text);
    virtual void preEditString(wl_resource *resource, uint32_t serial, const char *text, const char* commit);
    virtual void preEditStyling(wl_resource *resource, uint32_t serial, uint32_t index, uint32_t length, uint32_t style);
    virtual void preEditCursor(wl_resource *resource, uint32_t serial, int32_t index);
    virtual void deleteSurroundingText(wl_resource *resource, uint32_t serial, int32_t index, uint32_t length);
    virtual void cursorPosition(wl_resource *resource, uint32_t serial, int32_t index, int32_t anchor);
    virtual void modifiersMap(wl_resource *resource, struct wl_array *map);
    virtual void keySym(wl_resource *resource, uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers);
    virtual void sendEntered(wl_resource* resource, wl_resource* surface);
    virtual void sendLeft(wl_resource* resource);
    virtual void sendInputPanelState(wl_resource* resource, uint32_t state);
    virtual void sendInputPanelRect(wl_resource* resource, int32_t x, int32_t y,
                                    uint32_t width, uint32_t height);
};

class WEBOS_COMPOSITOR_EXPORT WaylandTextModel : public QObject {

    Q_OBJECT

public:
    WaylandTextModel(WaylandTextModelFactory *, struct wl_client *client, struct wl_resource *resource, uint32_t id);
    ~WaylandTextModel();

    void commitString(uint32_t serial, const char *text);
    void preEditString(uint32_t serial, const char *text, const char* commit);
    void preEditStyling(uint32_t serial, uint32_t index, uint32_t length, uint32_t style);
    void preEditCursor(uint32_t serial, int32_t index);
    void deleteSurroundingText(uint32_t serial, int32_t index, uint32_t length);
    void cursorPosition(uint32_t serial, int32_t index, int32_t anchor);
    void modifiersMap(struct wl_array *map);
    void keySym(uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers);

    void sendEntered();
    void sendLeft();

    static void textModelSetSurroundingText(struct wl_client *client, struct wl_resource *resource, const char *text, uint32_t cursor, uint32_t anchor);
    static void textModelActivate(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *seat, struct wl_resource *surface);
    static void textModelDeactivate(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat);
    static void textModelReset(struct wl_client *client, struct wl_resource *resource, uint32_t serial);
    static void textModelSetCursorRectangle(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
    static void textModelSetContentType(struct wl_client *client, struct wl_resource *resource, uint32_t hint, uint32_t purpose);
    static void textModelSetEnterKeyType(struct wl_client *client, struct wl_resource *resource, uint32_t enter_key_type);
    static void textModelInvokeAction(struct wl_client *client, struct wl_resource *resource, uint32_t button, uint32_t index);
    static void textModelCommit(struct wl_client *client, struct wl_resource *resource);
    static void textModelSetMaxTextLength(struct wl_client *client, struct wl_resource *resource, uint32_t length);
    static void textModelSetPlatformData(struct wl_client *client, struct wl_resource *resource, const char *text);
    static void textModelShowInputPanel(struct wl_client *client, struct wl_resource *resource);
    static void textModelHideInputPanel(struct wl_client *client, struct wl_resource *resource);
    static void textModelSetInputPanelRect(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, uint32_t width, uint32_t height);
    static void textModelResetInputPanelRect(struct wl_client *client, struct wl_resource *resource);

    static void destroyTextModel(struct wl_resource *resource);

    wl_resource* handle() { return m_resource; }

    bool isActive() const { return m_active; }

    bool isAllowed() const { return m_inputMethod && m_inputMethod->allowed(); }
    WaylandInputMethod *inputMethod() const { return m_inputMethod; }
    void setInputMethod(WaylandInputMethod *method, WebOSSurfaceItem *item);
    WaylandTextModelFactory *factory() const { return m_factory; }
    void setDelegate(WaylandTextModelDelegate *delegate);

public slots:
    void sendInputPanelState(const WaylandInputPanel::InputPanelState state) const;
    void sendInputPanelRect(const QRect& geometry) const;
    void handleActiveFocusChanged();

signals:
    void activated();
    void deactivated();
    void destroyed();
    void reset(uint32_t serial);
    void contentTypeChanged(uint32_t hint, uint32_t purpose);
    void enterKeyTypeChanged(uint32_t enter_key_type);
    void surroundingTextChanged(const QString& text, uint32_t cursor, uint32_t anchor);
    void commit();
    void showInputPanel();
    void hideInputPanel();
    void actionInvoked(uint32_t button, uint32_t index);
    void maxTextLengthChanged(uint32_t length);
    void platformDataChanged(const QString& text);

private:
    static const struct text_model_interface textModelImplementation;

    QPointer<WaylandInputMethod> m_inputMethod;
    WaylandInputMethodContext* m_context;
    struct ::wl_resource* m_resource;
    struct ::wl_resource* m_surface;
    bool m_active;
    WaylandTextModelFactory *m_factory;
    QRect m_preferredPanelRect;
    QScopedPointer<WaylandTextModelDelegate> m_delegate;
};
#endif
