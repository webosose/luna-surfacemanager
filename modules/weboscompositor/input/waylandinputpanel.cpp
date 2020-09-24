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

#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <QtWaylandCompositor/private/qwaylandsurface_p.h>
#include <QDebug>
#include <QWaylandCompositor>

#include "webossurfaceitem.h"
#include "waylandinputpanel.h"

const struct input_panel_surface_interface WaylandInputPanelSurface::inputPanelSurfaceImplementation = {
    WaylandInputPanelSurface::setTopLevel
};

WaylandInputPanelSurface::WaylandInputPanelSurface(WebOSSurfaceItem* item, wl_client* client, uint32_t id)
    : m_surfaceItem(item)
    , m_client(client)
    , m_resource(0)
    , m_mapped(false)
{
    // This will make the current qml parts work... not sure if we want to handle it like this
    // for eva...
    m_surfaceItem->setType("_WEBOS_WINDOW_TYPE_KEYBOARD", true);
    m_surfaceItem->setGrabKeyboardFocusOnClick(false);
    m_resource = wl_client_add_object(client, &input_panel_surface_interface, &inputPanelSurfaceImplementation, id, this);
    m_resource->destroy = WaylandInputPanelSurface::destroyInputPanelSurface;

    if (m_surfaceItem->surface()) {
        connect(m_surfaceItem->surface(), SIGNAL(hasContentChanged()), this, SLOT(onSurfaceMapped()));
        connect(m_surfaceItem->surface(), SIGNAL(hasContentChanged()), this, SLOT(onSurfaceUnmapped()));
        connect(m_surfaceItem->surface(), &QWaylandSurface::sizeChanged, this, &WaylandInputPanelSurface::sizeChanged);
        connect(m_surfaceItem->surface(), &QWaylandSurface::destroyed, this, &WaylandInputPanelSurface::onSurfaceDestroyed);
    }
}

WaylandInputPanelSurface::~WaylandInputPanelSurface()
{
    if (m_resource) {
        wl_resource_destroy(m_resource);
        m_resource = NULL;
    }
}

void WaylandInputPanelSurface::destroyInputPanelSurface(struct wl_resource* resource)
{
    WaylandInputPanelSurface* that = static_cast<WaylandInputPanelSurface*>(resource->data);
    that->m_resource = 0;
}

QWaylandSurface* WaylandInputPanelSurface::surface()
{
    return m_mapped ? m_surfaceItem->surface() : 0;
}

void WaylandInputPanelSurface::onSurfaceMapped()
{
    if (!m_surfaceItem->surface() || !m_surfaceItem->surface()->hasContent())
        return;

    if (!m_mapped) {
        m_mapped = true;
        emit mapped();
    }
}

void WaylandInputPanelSurface::onSurfaceUnmapped()
{
    if (!m_surfaceItem->surface() || m_surfaceItem->surface()->hasContent())
        return;

    if (m_mapped) {
        m_mapped = false;
        emit unmapped();
    }
}

void WaylandInputPanelSurface::onSurfaceDestroyed(QObject *object)
{
    Q_UNUSED(object);
    if (m_mapped) {
        m_mapped = false;
        emit unmapped();
    }
    deleteLater();
}

void WaylandInputPanelSurface::setTopLevel(struct wl_client *client, struct wl_resource *resource, uint32_t position)
{
    Q_UNUSED(client);
    Q_UNUSED(resource);
    Q_UNUSED(position);
    qDebug() << "Configuring the location not supported" << position;
}

//////////////////////////////

const struct input_panel_interface WaylandInputPanel::inputPanelImplementation = {
    WaylandInputPanel::getInputPanelSurface
};

WaylandInputPanel::WaylandInputPanel(struct wl_client *client, uint32_t id)
    : m_resource(0)
    , m_client(client)
    , m_activeSurface(0)
    , m_state(InputPanelHidden)
{
    m_resource = wl_client_add_object(client, &input_panel_interface, &inputPanelImplementation, id, this);
    m_resource->destroy = WaylandInputPanel::destroyInputPanel;
}

WaylandInputPanel::~WaylandInputPanel()
{
}

void WaylandInputPanel::destroyInputPanel(struct wl_resource* resource)
{
    WaylandInputPanel* that = static_cast<WaylandInputPanel*>(resource->data);
    that->m_resource = NULL;
    delete that;
}

void WaylandInputPanel::getInputPanelSurface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource)
{
    WaylandInputPanel* that = static_cast<WaylandInputPanel*>(resource->data);
    QWaylandSurface* qwls = QWaylandSurface::fromResource(surface_resource);
    QWaylandQuickSurface *qsurface = static_cast<QWaylandQuickSurface *>(qwls);
    WebOSSurfaceItem* ipsi = qobject_cast<WebOSSurfaceItem*>(qsurface->surfaceItem());

    qDebug() << "wl_surface@" << surface_resource->object.id;
    if (!ipsi) {
        wl_resource_post_error(surface_resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "input panel surface not found");
        return;
    }

    WaylandInputPanelSurface* surface = new WaylandInputPanelSurface(ipsi, client, id);
    connect(surface, &WaylandInputPanelSurface::mapped, that, &WaylandInputPanel::onInputPanelSurfaceMapped);
    connect(surface, &WaylandInputPanelSurface::unmapped, that, &WaylandInputPanel::onInputPanelSurfaceUnmapped);
}

void WaylandInputPanel::onInputPanelSurfaceMapped()
{
    WaylandInputPanelSurface *surface = qobject_cast<WaylandInputPanelSurface *>(sender());

    m_surfaces << surface;
    updateActiveInputPanelSurface(qobject_cast<WaylandInputPanelSurface *>(surface));
}

void WaylandInputPanel::onInputPanelSurfaceUnmapped()
{
    WaylandInputPanelSurface *surface = qobject_cast<WaylandInputPanelSurface *>(sender());

    m_surfaces.removeAll(surface);
    if (surface == m_activeSurface)
        updateActiveInputPanelSurface();
}

void WaylandInputPanel::setInputPanelSize(const QSize& size)
{
    if (m_inputPanelSize != size) {
        m_inputPanelSize = size;
        emit inputPanelSizeChanged(m_inputPanelSize);
    }
}

void WaylandInputPanel::updateActiveInputPanelSurface(WaylandInputPanelSurface *surface)
{
    WaylandInputPanelSurface *target = 0;

    if (surface) {
        target = surface;
        qDebug() << "setting active inputPanelSurface" << target;
    } else {
        // surface arg can be NULL which means to select one appropriately
        // where it exists in m_surfaces list and is mapped currently
        Q_FOREACH(WaylandInputPanelSurface *s, m_surfaces) {
            if (s && s->surface()) {
                target = s;
                qDebug() << "auto-selected active inputPanelSurface" << target;
                break;
            }
        }
    }

    if (m_activeSurface != target) {
        qInfo() << "changing active inputPanelSurface" << m_activeSurface << "->" << target;
        if (m_activeSurface)
            disconnect(m_activeSurface, &WaylandInputPanelSurface::sizeChanged, this, &WaylandInputPanel::updateInputPanelSize);
        if (target)
            connect(target, &WaylandInputPanelSurface::sizeChanged, this, &WaylandInputPanel::updateInputPanelSize);
        m_activeSurface = target;
    }

    updateInputPanelState();
    updateInputPanelSize();
}

void WaylandInputPanel::updateInputPanelState()
{
    InputPanelState state = InputPanelHidden;

    // Considered as shown only if the active input panel surface has a valid size
    if (m_activeSurface && m_activeSurface->surface())
        state = m_activeSurface->surface()->size().isValid() ? InputPanelShown : InputPanelHidden;

    qDebug() << "activeSurface:" << m_activeSurface << m_state << "->" << state;

    if (m_state != state) {
        m_state = state;
        emit reportPanelState(m_state);
        if (m_state == InputPanelShown && m_rect.isValid())
            emit reportPanelRect(m_rect);
    }
}

void WaylandInputPanel::updateInputPanelSize()
{
    if (m_activeSurface && m_activeSurface->surface())
        setInputPanelSize(m_activeSurface->surface()->size());
    else
        setInputPanelSize(QSize());
}

void WaylandInputPanel::setInputPanelRect(const QRect& rect)
{
    if (m_rect != rect) {
        m_rect = rect;
        emit inputPanelRectChanged(m_rect);
        if (m_state == InputPanelShown)
            emit reportPanelRect(m_rect);
    }
}

QSize WaylandInputPanel::inputPanelSize() const
{
    if (m_activeSurface && m_activeSurface->surface())
        return m_activeSurface->surfaceItem()->size().toSize();
    return QSize();
}
