// Copyright (c) 2013-2019 LG Electronics, Inc.
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

#include <QWaylandCompositor>
#include <QWaylandInputDevice>
#include <QWaylandSurface>
#include <QWaylandSurfaceItem>
#include <QtCompositor/private/qwlsurface_p.h>
#include <QDebug>
#include <QRect>

#include "waylandtextmodel.h"
#include "waylandtextmodelfactory.h"
#include "waylandinputmethodcontext.h"

#include "webossurfaceitem.h"

const struct text_model_interface WaylandTextModel::textModelImplementation = {
    WaylandTextModel::textModelSetSurroundingText,
    WaylandTextModel::textModelActivate,
    WaylandTextModel::textModelDeactivate,
    WaylandTextModel::textModelReset,
    WaylandTextModel::textModelSetCursorRectangle,
    WaylandTextModel::textModelSetContentType,
    WaylandTextModel::textModelSetEnterKeyType,
    WaylandTextModel::textModelInvokeAction,
    WaylandTextModel::textModelCommit,
    WaylandTextModel::textModelSetMaxTextLength,
    WaylandTextModel::textModelSetPlatformData,
    WaylandTextModel::textModelShowInputPanel,
    WaylandTextModel::textModelHideInputPanel,
    WaylandTextModel::textModelSetInputPanelRect,
    WaylandTextModel::textModelResetInputPanelRect,
};

WaylandTextModel::WaylandTextModel(WaylandTextModelFactory *factory, struct wl_client *client, struct wl_resource *resource, uint32_t id)
    : m_inputMethod(nullptr)
    , m_context(0)
    , m_surface(0)
    , m_active(false)
    , m_factory(factory)
{
    qDebug() << this << client << resource;
    m_resource = wl_client_add_object(client, &text_model_interface, &textModelImplementation, id, this);
    m_resource->destroy = WaylandTextModel::destroyTextModel;
    wl_signal_init(&m_resource->destroy_signal);
}

WaylandTextModel::~WaylandTextModel()
{
    qDebug() << m_resource << this;
    m_resource = NULL;
}

void WaylandTextModel::commitString(uint32_t serial, const char *text)
{
    qDebug() << QString(text);
    text_model_send_commit_string(m_resource, serial, text);
}

void WaylandTextModel::preEditString(uint32_t serial, const char *text, const char* commit)
{
    qDebug() << QString(text) << QString(commit);
    text_model_send_preedit_string(m_resource, serial, text, commit);
}

void WaylandTextModel::preEditStyling(uint32_t serial, uint32_t index, uint32_t length, uint32_t style)
{
    text_model_send_preedit_styling(m_resource, serial, index, length, style);
}

void WaylandTextModel::preEditCursor(uint32_t serial, int32_t index)
{
    text_model_send_preedit_cursor(m_resource, serial, index);
}

void WaylandTextModel::deleteSurroundingText(uint32_t serial, int32_t index, uint32_t length)
{
    qDebug() << index << length;
    text_model_send_delete_surrounding_text(m_resource, serial, index, length);
}

void WaylandTextModel::cursorPosition(uint32_t serial, int32_t index, int32_t anchor)
{
    text_model_send_cursor_position(m_resource, serial, index, anchor);
}

void WaylandTextModel::modifiersMap(struct wl_array *map)
{
    text_model_send_modifiers_map(m_resource, map);
}

void WaylandTextModel::keySym(uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers)
{
    text_model_send_keysym(m_resource, serial, time, sym, state, modifiers);
}

void WaylandTextModel::sendEntered()
{
    text_model_send_enter(m_resource, m_surface);
}

void WaylandTextModel::sendLeft()
{
    m_active = false;
    text_model_send_leave(m_resource);
}

void WaylandTextModel::sendInputPanelState(const WaylandInputPanel::InputPanelState state) const
{
    text_model_send_input_panel_state(m_resource, state);
}

void WaylandTextModel::sendInputPanelRect(const QRect& geometry) const
{
    text_model_send_input_panel_rect(m_resource, geometry.x(), geometry.y(), geometry.width(), geometry.height());
}

void WaylandTextModel::textModelSetSurroundingText(struct wl_client *client, struct wl_resource *resource, const char *text, uint32_t cursor, uint32_t anchor)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    emit that->surroundingTextChanged(QString(text), cursor, anchor);

}

void WaylandTextModel::setInputMethod(WaylandInputMethod *method)
{
    Q_ASSERT(method);

    // NOTE: m_inputMethod goes to NULL as a QPointer, when only IME Server is down.
    // In that case, context is also destroyed. So we can create new context without worrying memory leak.
    if (!m_inputMethod)
        new WaylandInputMethodContext(method, this); // context has a lifecycle which is bound to wayland resource

    m_inputMethod = method;

    // m_preferredPanelRect can be set before m_inputMethod
    m_inputMethod->setPreferredPanelRect(m_preferredPanelRect);
}

void WaylandTextModel::textModelActivate(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *seat, struct wl_resource *surface)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    QWaylandSurface *surfaceRequested = QtWayland::Surface::fromResource(surface)->waylandSurface();
    QWaylandQuickSurface *qs = static_cast<QWaylandQuickSurface *>(surfaceRequested);
    WebOSSurfaceItem* wsi = qobject_cast<WebOSSurfaceItem *>(qs->surfaceItem());

    if (!wsi) {
        qWarning() << "Unexpected:" << that << surfaceRequested << "has no WebOSSurfaceItem";
        return;
    }

    qDebug() << "activation request for" << surfaceRequested << wsi;

    WaylandInputMethod *method = that->factory()->findInputMethod(wsi->displayId());
    if (!method) {
        qWarning() << "No input method found for display" << wsi->displayId();
        return;
    }

    // Whenever activating text model, update input method
    that->setInputMethod(method);

    if (!that->isAllowed()) {
        qWarning() << "activation declined as IME is not allowed at the moment";
        return;
    }

    QWaylandSurface *surfaceFocused = that->m_inputMethod->inputDevice()->keyboardFocus();
    if (surfaceFocused != surfaceRequested) {
        qWarning() << "activation declined for non-focused surface:" << surfaceRequested << "focused:" << surfaceFocused;
        return;
    }

    if (!that->isActive()) {
        that->m_surface = surface;
        that->m_active = true;

        QWaylandSurfaceItem *item = static_cast<QWaylandSurfaceItem *>(surfaceRequested->views().first());
        connect(item, &QQuickItem::activeFocusChanged, that, &WaylandTextModel::handleActiveFocusChanged, Qt::UniqueConnection);

        emit that->activated();
    } else {
        qDebug() << that << "already active for" << surfaceRequested;
    }
}

void WaylandTextModel::textModelDeactivate(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    if (that->isActive()) {
        that->m_active = false;
        emit that->deactivated();
    }

    if (that->handle())
        wl_resource_destroy(that->handle());
}

void WaylandTextModel::textModelReset(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    if (that->isActive()) {
        emit that->reset(serial);
    }
}

void WaylandTextModel::textModelSetCursorRectangle(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

void WaylandTextModel::textModelSetContentType(struct wl_client *client, struct wl_resource *resource, uint32_t hint, uint32_t purpose)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    emit that->contentTypeChanged(hint, purpose);
}

void WaylandTextModel::textModelSetEnterKeyType(struct wl_client *client, struct wl_resource *resource, uint32_t enter_key_type)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    emit that->enterKeyTypeChanged(enter_key_type);
}

void WaylandTextModel::textModelInvokeAction(struct wl_client *client, struct wl_resource *resource, uint32_t button, uint32_t index)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    if (that->isActive()) {
        emit that->actionInvoked(button, index);
    }
}

void WaylandTextModel::textModelCommit(struct wl_client *client, struct wl_resource *resource)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    if (that->isActive()) {
        emit that->commit();
    }
}

void WaylandTextModel::textModelSetMaxTextLength(struct wl_client *client, struct wl_resource *resource, uint32_t length)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    emit that->maxTextLengthChanged(length);
}

void WaylandTextModel::textModelSetPlatformData(struct wl_client *client, struct wl_resource *resource, const char *text)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    emit that->platformDataChanged(QString(text));
}

void WaylandTextModel::textModelShowInputPanel(struct wl_client *client, struct wl_resource *resource)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    if (that->isActive()) {
        emit that->showInputPanel();
    }
}

void WaylandTextModel::textModelHideInputPanel(struct wl_client *client, struct wl_resource *resource)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    if (that->isActive()) {
        emit that->hideInputPanel();
    }
}

void WaylandTextModel::textModelSetInputPanelRect(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    that->m_preferredPanelRect = QRect(x, y, width, height);
    if (that->m_inputMethod)
        that->m_inputMethod->setPreferredPanelRect(that->m_preferredPanelRect);
    qDebug() << "Client request InputPanelRect:" << that->m_preferredPanelRect;
}

void WaylandTextModel::textModelResetInputPanelRect(struct wl_client *client, struct wl_resource *resource)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    that->m_preferredPanelRect = QRect();
    if (that->m_inputMethod)
        that->m_inputMethod->resetPreferredPanelRect();
}

void WaylandTextModel::destroyTextModel(struct wl_resource *resource)
{
    WaylandTextModel* that = static_cast<WaylandTextModel*>(resource->data);
    if (that->m_inputMethod)
        that->m_inputMethod->resetPreferredPanelRect();
    that->m_active = false;
    emit that->destroyed();
    delete that;
}

void WaylandTextModel::handleActiveFocusChanged()
{
    QWaylandSurfaceItem *item = qobject_cast<QWaylandSurfaceItem *>(sender());

    if (item && !item->hasActiveFocus() && m_active && m_inputMethod->isActiveModel(this)) {
        qWarning() << "deactivate the current context as the surface item initiated it gets unfocused" << item;
        m_inputMethod->deactivate();
    }
}
