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

#include <QDebug>

#include <QWaylandSeat>
#include "waylandinputmethod.h"
#include "waylandinputmethodcontext.h"
#include "waylandinputpanel.h"
#include "waylandinputmethodmanager.h"

#include "weboscorecompositor.h"

const struct input_method_interface WaylandInputMethod::inputMethodImplementation = {
    WaylandInputMethod::setDisplayId
};

WaylandInputMethod::WaylandInputMethod()
{
    qFatal("Constructor only for the purpose of registering the QML type");
}

WaylandInputMethod::WaylandInputMethod(QWaylandCompositor* compositor)
    : m_compositor(compositor)
    , m_resource(0)
    , m_client(nullptr)
    , m_activeContext(0)
    , m_inputPanel(0)
    , m_hasPreferredPanelRect(false)
    , m_inputMethodManager(0)
    , m_allowed(true)
    , m_displayId(-1)
    , m_targetSurfaceItem(nullptr)
{
}

void WaylandInputMethod::initialize()
{
    m_inputMethodManager = new WaylandInputMethodManager(this);
    connect(this, &WaylandInputMethod::inputMethodBound, m_inputMethodManager, &WaylandInputMethodManager::onInputMethodAvaliable, Qt::QueuedConnection);
}

WaylandInputMethod::~WaylandInputMethod()
{
    delete m_inputMethodManager;
}

void WaylandInputMethod::binding(struct wl_client *client, uint32_t id, wl_resource_destroy_func_t destroy)
{
    wl_resource* resource = wl_client_add_object(client, &input_method_interface, &inputMethodImplementation, id, this);
    resource->destroy = destroy;
    setHandle(resource);
    setClient(client);
    emit inputMethodBound(true);
}

void WaylandInputMethod::unbinding()
{
    setDisplayId(-1);
    setHandle(nullptr);
    setClient(nullptr);
    deactivate();
    emit inputMethodBound(false);
}

void WaylandInputMethod::handleDestroy()
{
    qInfo() << "WaylandInputMethod::handleDestroy" << this << displayId();
    unbinding();
    // Delete gracefully after some queued signals are processed
    deleteLater();
}

void WaylandInputMethod::destroyInputMethod(struct wl_resource* resource)
{
    WaylandInputMethod* that = static_cast<WaylandInputMethod*>(resource->data);
    that->handleDestroy();
}

void WaylandInputMethod::setDisplayId(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    WaylandInputMethod* that = static_cast<WaylandInputMethod*>(resource->data);
    qInfo() << "setDisplayId" << id << "for" << that;
    that->setDisplayId(id);
}

void WaylandInputMethod::setDisplayId(const uint32_t displayId)
{
    if (m_displayId != displayId) {
        qInfo() << this << "set to displayId" << displayId;
        m_displayId = displayId;
        emit displayIdChanged();
    }
}

QWaylandSeat *WaylandInputMethod::inputDevice()
{
    return static_cast<WebOSCoreCompositor *>(m_compositor)->keyboardDeviceForDisplayId(displayId());
}

void WaylandInputMethod::contextActivated()
{
    WaylandInputMethodContext* context = qobject_cast<WaylandInputMethodContext*>(sender());
    qDebug() << m_activeContext << context;
    if (m_activeContext == context) {
        return;
    }
    m_activeContext = context;
    emit activeChanged();
}

void WaylandInputMethod::contextDeactivated()
{
    WaylandInputMethodContext* context = qobject_cast<WaylandInputMethodContext*>(sender());
    qDebug() << m_activeContext << context << sender();
    if (m_activeContext != sender()) {
        return;
    }
    m_activeContext = NULL;
    emit activeChanged();
}

void WaylandInputMethod::deactivate()
{
    if (m_activeContext) {
        m_activeContext->deactivate();
    }
}

bool WaylandInputMethod::isActiveModel(WaylandTextModel *model) const
{
    return m_activeContext && m_activeContext->textModel() == model;
}

void WaylandInputMethod::setAllowed(bool allowed)
{
    if (m_allowed != allowed) {
        m_allowed = allowed;
        emit allowedChanged();

        if (!m_allowed)
            deactivate();
    }
}

QRect WaylandInputMethod::panelRect() const
{
    if (m_inputPanel)
        return m_inputPanel->inputPanelRect();

    qWarning() << "No input panel created, panel rect is empty";
    return QRect();
}

void WaylandInputMethod::setPanelRect(const QRect& rect)
{
    if (m_inputPanel)
        m_inputPanel->setInputPanelRect(rect);
    else
        qWarning() << "No input panel created, unable to set panel rect";
}

void WaylandInputMethod::setPreferredPanelRect(const QRect& rect)
{
    if (m_preferredPanelRect != rect) {
        m_preferredPanelRect = rect;
        emit preferredPanelRectChanged();
        setHasPreferredPanelRect(true);
    }
}

void WaylandInputMethod::resetPreferredPanelRect()
{
    setHasPreferredPanelRect(false);
    m_preferredPanelRect.setRect(0, 0, 0, 0);
}

void WaylandInputMethod::setHasPreferredPanelRect(const bool flag)
{
    if (m_hasPreferredPanelRect != flag) {
        m_hasPreferredPanelRect = flag;
        emit hasPreferredPanelRectChanged();
    }
}

void WaylandInputMethod::setTargetSurfaceItem(WebOSSurfaceItem *item)
{
    if (m_targetSurfaceItem != item) {
        m_targetSurfaceItem = item;
        emit setTargetSurfaceItemChanged();
    }
}

void WaylandInputMethod::setInputPanel(WaylandInputPanel *panel)
{
    qInfo() << "panel" << panel << "set to method" << this;
    m_inputPanel = panel;

    connect(m_inputPanel, &WaylandInputPanel::inputPanelRectChanged, this, &WaylandInputMethod::panelRectChanged);
}
