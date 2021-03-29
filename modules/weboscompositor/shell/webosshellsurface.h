// Copyright (c) 2013-2021 LG Electronics, Inc.
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

#ifndef WEBOSSHELLSURFACE_H
#define WEBOSSHELLSURFACE_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QObject>
#include <QMap>
#include <wayland-server.h>
#include <wayland-webos-shell-server-protocol.h>

#include "webossurfaceitem.h"

class WebOSCoreCompositor;
class QWaylandSurface;

class WEBOS_COMPOSITOR_EXPORT WebOSShell : public QObject {

    Q_OBJECT

public:
    WebOSShell(WebOSCoreCompositor* compositor);
    static void bind_func(struct wl_client *client, void *data, uint32_t version, uint32_t id);
    static void destroy_func(struct wl_resource* resource);
    void bind(struct wl_client *client, int version);
    void unbind(struct wl_client *client);
    int getVersion(struct wl_client *client);

private:
    // methods
    static void get_shell_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface);
    static void get_system_pip(struct wl_client *client, struct wl_resource *resource);

private slots:
    void registerSurfaceChange(QWaylandSurface* prev, QWaylandSurface* current);

private:
    // variables
    static const struct wl_webos_shell_interface shell_interface;
    WebOSCoreCompositor* m_compositor;
    QWaylandSurface* m_previousFullscreenSurface;
    QMap<struct wl_client *, int> m_versionMap;
};

class WebOSShellSurface : public QObject {

    Q_OBJECT

public:
    WebOSShellSurface(struct wl_client* client, uint32_t id, WebOSSurfaceItem* surface, wl_resource* owner, int version);
    ~WebOSShellSurface();

    int version() const { return m_version; }

    void setState(Qt::WindowState state);
    void prepareState(Qt::WindowState state);
    void close();

    WebOSSurfaceItem::LocationHints locationHint() { return m_locationHint; }
    WebOSSurfaceItem::KeyMasks keyMask() { return m_keyMask; }
    Qt::WindowState state() { return m_state; }

    void setPosition(const QPointF& pos);

    QVariantMap properties() const;
    QVariant property(const QString &propertyName) const;
    void setProperty(const QString &name, const QVariant &value, bool notify = true);

    QString addon() const { return m_addon; }
    void setAddonStatus(WebOSSurfaceItem::AddonStatus status);

public slots:
    void exposed(const QRegion& region);

signals:
    void locationHintChanged();
    void keyMaskChanged();
    void stateChangeRequested(Qt::WindowState s);
    void propertiesChanged(const QVariantMap &properties, const QString &name, const QVariant &value);
    void addonChanged();

private:
    // methods
    static void destroyShellSurface(struct wl_resource* resource);
    static void set_location_hint(struct wl_client *client, struct wl_resource *resource, uint32_t hint);
    static void set_state(struct wl_client *client, struct wl_resource *resource, uint32_t state);
    static void set_property(struct wl_client *client, struct wl_resource *resource, const char *name, const char *value);
    static void set_key_mask(struct wl_client *client, struct wl_resource *resource, uint32_t webos_key);
    static void set_addon(struct wl_client *client, struct wl_resource *resource, const char *path);
    static void reset_addon(struct wl_client *client, struct wl_resource *resource);

    void addValue(int32_t value, wl_array* to);
    bool validExposeRect(const QRect& rect);

    /*!
     * Emits the signal if the given Q_PROPERTY definition exists and if it has
     * a NOTIFY signal assigned to it. Used by WebOSShellSurface to emit quick
     * access property signal for certain window properties.
     */
    void emitSurfaceConvenienceSignal(const QString& property);

    // variables
    int m_version = -1;
    static const struct wl_webos_shell_surface_interface shell_surface_interface;
    wl_resource* m_shellSurface;
    WebOSSurfaceItem::LocationHints m_locationHint;
    WebOSSurfaceItem::KeyMasks m_keyMask;
    Qt::WindowState m_state;
    Qt::WindowState m_preparedState;
    wl_resource* m_owner;
    QVariantMap m_properties;
    QRegion m_exposed;
    WebOSSurfaceItem* m_surface;
    QString m_addon;
};
#endif
