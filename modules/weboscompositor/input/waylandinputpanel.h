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

#ifndef WAYLANDINPUTPANEL_H
#define WAYLANDINPUTPANEL_H

#include <QObject>
#include <QRect>

#include <wayland-server.h>
#include <wayland-input-method-server-protocol.h>

class QWaylandCompositor;
class WebOSSurfaceItem;
class QWaylandSurface;

class WaylandInputPanelSurface : public QObject {
    Q_OBJECT

public:
    WaylandInputPanelSurface(WebOSSurfaceItem* item, wl_client* client, uint32_t id);
    ~WaylandInputPanelSurface();

    static void destroyInputPanelSurface(struct wl_resource* resource);
    static void setTopLevel(struct wl_client *client, struct wl_resource *resource, uint32_t position);
    static const struct input_panel_surface_interface inputPanelSurfaceImplementation;

    QWaylandSurface* surface(); // returns surface mapped
    WebOSSurfaceItem* surfaceItem() { return m_surfaceItem; }

signals:
    void mapped();
    void unmapped();
    void sizeChanged();

private slots:
    void onSurfaceMapped();
    void onSurfaceUnmapped();
    void onSurfaceDestroyed(QObject *object);

private:
    WebOSSurfaceItem* m_surfaceItem;
    wl_client* m_client;
    wl_resource* m_resource;
    bool m_mapped;
};


/*!
 * Provides the input_panel interface for the clients
 */
class WaylandInputPanel : public QObject {

    Q_OBJECT

public:
    enum InputPanelState {
        InputPanelHidden = 0,
        InputPanelShown = 1
    };
    Q_ENUM(InputPanelState)

    WaylandInputPanel(struct wl_client *client, uint32_t id);
    ~WaylandInputPanel();

    static void bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);
    static void destroyInputPanel(struct wl_resource* resource);

    static void getInputPanelSurface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource);

    struct wl_client * client() const { return m_client; }
    QRect inputPanelRect() const { return m_rect; }
    void setInputPanelRect(const QRect& rect);
    QSize inputPanelSurfaceSize() const;

signals:
    void reportPanelState(InputPanelState state);
    void reportPanelRect(const QRect& rect);
    void inputPanelRectChanged(const QRect& rect);
    void inputPanelSurfaceSizeChanged(const QSize& size);

private:
    // methods
    void updateActiveInputPanelSurface(WaylandInputPanelSurface *surface = 0);
    void setInputPanelSurfaceSize(const QSize& size);

private slots:
    void onInputPanelSurfaceMapped();
    void onInputPanelSurfaceUnmapped();
    void updateInputPanelSurfaceSize();
    void updateInputPanelState();

private:
    // variables
    static const struct input_panel_interface inputPanelImplementation;
    QWaylandCompositor* m_compositor;
    wl_resource* m_resource;
    wl_client *m_client;
    QList<WaylandInputPanelSurface*> m_surfaces;
    WaylandInputPanelSurface *m_activeSurface;

    InputPanelState m_state;
    QRect m_rect;
    QSize m_inputPanelSurfaceSize;
};

#endif //WAYLANDINPUTPANEL_H
