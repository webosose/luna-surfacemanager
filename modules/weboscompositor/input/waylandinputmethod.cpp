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

#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <QWaylandCompositor>
#include <QDebug>

#include "waylandinputmethod.h"
#include "waylandinputmethodcontext.h"
#include "waylandtextmodelfactory.h"
#include "waylandtextmodel.h"
#include "waylandinputpanel.h"
#include "waylandinputmethodmanager.h"

WaylandInputMethod::WaylandInputMethod(QWaylandCompositor* compositor)
    : m_compositor(compositor)
    , m_factory(0)
    , m_resource(0)
    , m_activeContext(0)
    , m_inputPanel(0)
    , m_hasPreferredPanelRect(false)
    , m_inputMethodManager(0)
    , m_allowed(true)
{
    m_factory = new WaylandTextModelFactory(compositor, this);
    wl_display_add_global(compositor->waylandDisplay(), &input_method_interface, this, WaylandInputMethod::bind);

    m_inputPanel = new WaylandInputPanel(compositor);
    m_inputMethodManager = new WaylandInputMethodManager(this);

    connect(m_inputPanel, &WaylandInputPanel::inputPanelRectChanged, this, &WaylandInputMethod::panelRectChanged);
    connect(m_inputPanel, &WaylandInputPanel::inputPanelSurfaceSizeChanged, this, &WaylandInputMethod::panelSurfaceSizeChanged);
    connect(this, &WaylandInputMethod::inputMethodBound, m_inputMethodManager, &WaylandInputMethodManager::onInputMethodAvaliable, Qt::QueuedConnection);
}

WaylandInputMethod::~WaylandInputMethod()
{
}

void WaylandInputMethod::bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    Q_UNUSED(version);
    WaylandInputMethod* that = static_cast<WaylandInputMethod*>(data);

    wl_resource* resource = wl_client_add_object(client, &input_method_interface, NULL, id, that);
    if (!that->m_resource) {
        that->m_resource = resource;
        resource->destroy = WaylandInputMethod::destroyInputMethod;
        emit that->inputMethodBound(true);
    } else {
        qWarning() << "trying to bind more than once";
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "interface object already bound");
        wl_resource_destroy(resource);
    }
}

void WaylandInputMethod::destroyInputMethod(struct wl_resource* resource)
{
    WaylandInputMethod* that = static_cast<WaylandInputMethod*>(resource->data);
    that->m_resource = NULL;
    that->deactivate();
    emit that->inputMethodBound(false);
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

QSize WaylandInputMethod::panelSurfaceSize() const
{
    if (m_inputPanel)
        return m_inputPanel->inputPanelSurfaceSize();

    qWarning() << "No active panel surface, surface size is empty";
    return QSize();
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
