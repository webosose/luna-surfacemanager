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

#include "webosshellsurface.h"
#include "weboscorecompositor.h"

#include <QDebug>
#include <QWaylandCompositor>
#include <QWaylandSurface>
#include <QtWaylandCompositor/private/qwaylandsurface_p.h>

const struct wl_webos_shell_interface WebOSShell::shell_interface = {
    WebOSShell::get_system_pip,
    WebOSShell::get_shell_surface
};

WebOSShell::WebOSShell(WebOSCoreCompositor* compositor)
    : m_compositor(compositor)
    , m_previousFullscreenSurface(0)
{
    wl_display_add_global(compositor->display(), &wl_webos_shell_interface, this, WebOSShell::bind_func);
    connect(compositor, SIGNAL(fullscreenSurfaceChanged(QWaylandSurface*, QWaylandSurface*)),
            this, SLOT(registerSurfaceChange(QWaylandSurface*, QWaylandSurface*)));
}

void WebOSShell::bind_func(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    WebOSShell* that = static_cast<WebOSShell *>(data);
    that->bind(client, version);
    wl_resource *shell = wl_client_add_object(client, &wl_webos_shell_interface, &shell_interface, id, data);
    shell->destroy = WebOSShell::destroy_func;
}

void WebOSShell::destroy_func(struct wl_resource* resource)
{
    WebOSShell* that = static_cast<WebOSShell *>(resource->data);
    that->unbind(wl_resource_get_client(resource));
}

void WebOSShell::bind(struct wl_client *client, int version)
{
    m_versionMap.insert(client, version);
}

void WebOSShell::unbind(struct wl_client *client)
{
    m_versionMap.take(client);
}

int WebOSShell::getVersion(struct wl_client *client)
{
    return m_versionMap.value(client, -1);
}

void WebOSShell::get_shell_surface(struct wl_client *client, struct wl_resource *shell_resource, uint32_t id, struct wl_resource *owner)
{
    WebOSShell* that = static_cast<WebOSShell *>(shell_resource->data);
    QWaylandSurface* surface = QWaylandSurface::fromResource(owner);
    if (surface == nullptr) {
        qWarning()  << "[QWaylandSurface] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "shell_resource :" << (shell_resource ? shell_resource : nullptr)
                    << "id :" << id
                    << "owner :" << (owner ? owner->object.id : 0);
        return;
    }

    WebOSSurfaceItem* item = WebOSSurfaceItem::getSurfaceItemFromSurface(surface);
    if (item == nullptr) {
        qWarning()  << "[WebOSSurfaceItem] Invalid pointer, "
                    << "surface :" << surface
                    << "client :" << (client ? client : nullptr)
                    << "shell_resource :" << (shell_resource ? shell_resource : nullptr)
                    << "id :" << id
                    << "owner :" << (owner ? owner->object.id : 0);
        return;
    }

    qDebug() << surface << item;

    new WebOSShellSurface(client, id, item, owner, that->getVersion(client));
}

void WebOSShell::get_system_pip(struct wl_client *client, struct wl_resource *resource)
{
    // Deprecated; We still need this for launching prebuilt applcations.
    // If you want to create a new request, you can reuse this.
    // FYI, in that case, you need to modify both
    // get_system_pip request in luna-surfacemanager-extensions, and this one.
}

void WebOSShell::registerSurfaceChange(QWaylandSurface* prev, QWaylandSurface* current)
{
    Q_UNUSED(current);

    if (prev == nullptr || current == nullptr) {
      qWarning()  << "[QWaylandSurface] Invalid pointer, "
                  << "prev :" << (prev ? prev : nullptr)
                  << "current :" << (current ? current : nullptr);
    }

    qDebug()  << "prev :" << (prev ? prev : nullptr)
              << "current :" << (current ? current : nullptr);

    m_previousFullscreenSurface = prev;
}

const struct wl_webos_shell_surface_interface WebOSShellSurface::shell_surface_interface = {
    WebOSShellSurface::set_location_hint,
    WebOSShellSurface::set_state,
    WebOSShellSurface::set_property,
    WebOSShellSurface::set_key_mask,
    WebOSShellSurface::set_addon,
    WebOSShellSurface::reset_addon
};

WebOSShellSurface::WebOSShellSurface(struct wl_client* client, uint32_t id, WebOSSurfaceItem* surface, wl_resource* owner, int version)
    : m_version(version)
    , m_locationHint(WebOSSurfaceItem::LocationHintCenter)
    , m_keyMask(WebOSSurfaceItem::KeyMaskDefault)
    , m_state(Qt::WindowNoState)
    , m_preparedState(Qt::WindowNoState)
    , m_owner(owner)
    , m_surface(surface)
{
    m_shellSurface = wl_client_add_object(client, &wl_webos_shell_surface_interface, &shell_surface_interface, id, this);

    if (m_shellSurface != nullptr) {
        m_shellSurface->destroy = WebOSShellSurface::destroyShellSurface;

        surface->setShellSurface(this);
        qDebug() << this << "for wl_surface@" << m_owner->object.id << "m_shellSurface:" << m_shellSurface;
    } else {
        qWarning()  << "[wl_resource] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "id :" << id
                    << "surface :" << (surface ? surface : nullptr)
                    << "owner :" << (owner ? owner->object.id : 0);
    }
}

WebOSShellSurface::~WebOSShellSurface()
{
    m_surface->resetShellSurface(this);
    if (m_shellSurface != nullptr) {
        qDebug() << this << "for wl_surface@" << m_owner->object.id << "m_shellSurface:" << m_shellSurface;
        wl_resource_destroy(m_shellSurface);
    } else {
        qWarning() << "[wl_resource] Invalid pointer";
    }
}

void WebOSShellSurface::destroyShellSurface(struct wl_resource* resource)
{
    if (resource == nullptr) {
        qWarning()  << "[wl_resource] Invalid pointer";
        return;
    }

    WebOSShellSurface* that = static_cast<WebOSShellSurface *>(resource->data);
    if (that == nullptr) {
        qWarning()  << "[WebOSShellSurface] Invalid pointer, "
                    << "resource :" << resource;
        return;
    }

    qDebug() << that << "for resource" << resource << "m_shellSurface:" << that->m_shellSurface;
    that->m_shellSurface = NULL;
}

static uint32_t waylandStateFromQtWindowState(Qt::WindowState state)
{
    switch (state) {
        case Qt::WindowNoState: return WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT;
        case Qt::WindowMinimized: return WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED;
        case Qt::WindowMaximized: return WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED;
        case Qt::WindowFullScreen: return WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN;
        default: return WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT;
    }
}

static Qt::WindowState qtWindowStateFromWaylandState(uint32_t state)
{
    switch (state) {
        case WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT: return  Qt::WindowNoState;
        case WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED: return Qt::WindowMinimized;
        case WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED: return Qt::WindowMaximized;
        case WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN: return Qt::WindowFullScreen;
        default: return Qt::WindowNoState;
    }
}

void WebOSShellSurface::prepareState(Qt::WindowState state)
{
    if (m_shellSurface && m_state != state) {
        qDebug() << m_surface << "m_preparedState" << m_preparedState << "->" << state;
        m_preparedState = state;
        wl_webos_shell_surface_send_state_about_to_change(m_shellSurface, waylandStateFromQtWindowState(state));
    }
}

void WebOSShellSurface::setState(Qt::WindowState state)
{
    if (m_shellSurface && (m_state != state || m_preparedState != Qt::WindowNoState)) {
        qDebug() << m_surface << "m_state" << m_state << "->" << state;
        m_state = state;
        m_preparedState = Qt::WindowNoState;
        wl_webos_shell_surface_send_state_changed(m_shellSurface, waylandStateFromQtWindowState(state));
    }
}

void WebOSShellSurface::set_location_hint(struct wl_client *client, struct wl_resource *resource, uint32_t hint)
{
    Q_UNUSED(client);
    if (resource == nullptr) {
        qWarning()  << "[wl_resource] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "hint :" << hint;
        return;
    }

    WebOSSurfaceItem::LocationHints newHint = (WebOSSurfaceItem::LocationHints)hint;
    WebOSShellSurface* that = static_cast<WebOSShellSurface*>(resource->data);
    if (that == nullptr) {
        qWarning()  << "[WebOSShellSurface] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "resource :" << resource
                    << "hint :" << hint;
        return;
    }

    if (that->m_locationHint != newHint) {
        that->m_locationHint = newHint;
        qDebug() << "changed" << that->m_locationHint;
        emit that->locationHintChanged();
    }
}

void WebOSShellSurface::set_key_mask(struct wl_client *client, struct wl_resource *resource, uint32_t webos_key)
{
    Q_UNUSED(client);
    if (resource == nullptr) {
        qWarning()  << "[wl_resource] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "webos_key :" << webos_key;
        return;
    }

    WebOSSurfaceItem::KeyMasks newKeyMasks = (WebOSSurfaceItem::KeyMasks)webos_key;
    WebOSShellSurface* that = static_cast<WebOSShellSurface*>(resource->data);
    if (that == nullptr) {
        qWarning()  << "[WebOSShellSurface] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "resource :" << resource
                    << "webos_key :" << webos_key;
        return;
    }

    if (that->m_keyMask != newKeyMasks) {
        qInfo() << "key mask changed. newKeyMasks = " << newKeyMasks << ", client : " << (client ? client : nullptr);
        that->m_keyMask = newKeyMasks;
        emit that->keyMaskChanged();
    }
}

void WebOSShellSurface::set_state(struct wl_client *client, struct wl_resource *resource, uint32_t state)
{
    Q_UNUSED(client);
    if (resource == nullptr) {
        qWarning()  << "[wl_resource] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "state :" << state;
        return;
    }

    Qt::WindowState newState = qtWindowStateFromWaylandState(state);
    WebOSShellSurface* that = static_cast<WebOSShellSurface*>(resource->data);
    if (that == nullptr) {
        qWarning()  << "[WebOSShellSurface] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "resource :" << resource
                    << "state :" << state;
        return;
    }

    if (!that->m_surface->isMapped()) {
        qWarning() << "Ignored for unmapped surface" << that->m_surface << that << that->m_state << newState;
        return;
    }

    if (that->m_state != newState) {
        qInfo() << "state change requested:" << that->m_state << "->" << newState
                     << that->m_surface << that->m_surface->appId();
        emit that->stateChangeRequested(newState);
    }
}

void WebOSShellSurface::close()
{
    if (m_shellSurface) {
        qDebug() << "m_shellSurface:" << m_shellSurface;
        wl_webos_shell_surface_send_close(m_shellSurface);
    }
}

void WebOSShellSurface::setPosition(const QPointF& pos)
{
    if (m_shellSurface)
        wl_webos_shell_surface_send_position_changed(m_shellSurface, pos.toPoint().x(), pos.toPoint().y());
}

QVariantMap WebOSShellSurface::properties() const
{
    return m_properties;
}

QVariant WebOSShellSurface::property(const QString &propertyName) const
{
    return m_properties.value(propertyName);
}

void WebOSShellSurface::setProperty(const QString &name, const QVariant &value, bool notify)
{
    if (value.isNull())
        m_properties.remove(name);
    else
        m_properties.insert(name, value);

    if (notify)
        emit propertiesChanged(m_properties, name, value);
}

void WebOSShellSurface::set_property(struct wl_client *client, struct wl_resource *resource, const char *name, const char *value)
{
    Q_UNUSED(client);
    if (resource == nullptr || name == nullptr || value == nullptr) {
        qWarning()  << "[wl_resource] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "name :" << (name ? name : nullptr)
                    << "value :" << (value ? value : nullptr);
        return;
    }

    WebOSShellSurface* that = static_cast<WebOSShellSurface*>(resource->data);
    if (that == nullptr) {
        qWarning()  << "[WebOSShellSurface] Invalid pointer, "
                    << "client :" << (client ? client : nullptr)
                    << "resource :" << resource
                    << "name :" << name
                    << "value :" << value;
        return;
    }

    QString key = QString::fromLatin1(name);
    QVariant newValue = QVariant(QString::fromUtf8(value));

    bool emitChange = that->property(key) != newValue;
    qDebug() << "set property (" << QString::fromLatin1(name) << "," << QVariant(QString::fromUtf8(value)) << ")"
                 << that->m_surface << that->m_surface->appId();
    that->setProperty(QString::fromLatin1(name), QVariant(QString::fromUtf8(value)));
    if (emitChange)
        that->emitSurfaceConvenienceSignal(key);
}

void WebOSShellSurface::emitSurfaceConvenienceSignal(const QString& key)
{
    const QMetaObject* mo = m_surface->metaObject();
    if (mo == nullptr) {
        qWarning()  << "[QMetaObject] Invalid pointer, "
                    << "key :" << key;
        return;
    }

    int propertyIndex = mo->indexOfProperty(key.toLatin1().data());
    QMetaProperty property = mo->property(propertyIndex);
    if (property.isValid() && property.hasNotifySignal()) {
        QMetaMethod signal = property.notifySignal();
        qDebug() << "emit" << signal.name();
        signal.invoke(m_surface);
    }
}

void WebOSShellSurface::exposed(const QRegion& region)
{
    if (!m_shellSurface)
        return;

    wl_array rects;
    wl_array_init(&rects);

    for (auto &r: region) {
        if (validExposeRect(r)) {
            addValue(r.x(), &rects);
            addValue(r.y(), &rects);
            addValue(r.width(), &rects);
            addValue(r.height(), &rects);
        } else {
            qWarning() << "Skipping expose rect" << r;
        }
    }

    // Add the terminating marker, see protocol defintion for more
    addValue(-1, &rects);
    wl_webos_shell_surface_send_exposed(m_shellSurface, &rects);
    wl_array_release(&rects);
}

void WebOSShellSurface::addValue(int32_t value, wl_array* to)
{
    int32_t* v = (int32_t*)wl_array_add(to, sizeof(int32_t));
    *v = value;
}

bool WebOSShellSurface::validExposeRect(const QRect& rect)
{
    return !(rect.x() < 0 ||
           rect.y() < 0 ||
           rect.width() < 0 ||
           rect.height() < 0);
}

void WebOSShellSurface::setAddonStatus(WebOSSurfaceItem::AddonStatus status)
{
    if (m_shellSurface && version() >= WL_WEBOS_SHELL_SURFACE_ADDON_STATUS_CHANGED_SINCE_VERSION) {
        wl_webos_shell_surface_addon_status addonStatus;
        switch (status) {
        case WebOSSurfaceItem::AddonStatusNull:
            addonStatus = WL_WEBOS_SHELL_SURFACE_ADDON_STATUS_NULL;
            break;
        case WebOSSurfaceItem::AddonStatusLoaded:
            addonStatus = WL_WEBOS_SHELL_SURFACE_ADDON_STATUS_LOADED;
            break;
        case WebOSSurfaceItem::AddonStatusDenied:
            addonStatus = WL_WEBOS_SHELL_SURFACE_ADDON_STATUS_DENIED;
            break;
        case WebOSSurfaceItem::AddonStatusError:
            addonStatus = WL_WEBOS_SHELL_SURFACE_ADDON_STATUS_ERROR;
            break;
        default:
            Q_UNREACHABLE();
            break;
        }

        wl_webos_shell_surface_send_addon_status_changed(m_shellSurface, addonStatus);
    }
}

void WebOSShellSurface::set_addon(struct wl_client *client, struct wl_resource *resource, const char *path)
{
    Q_UNUSED(client);
    WebOSShellSurface* that = static_cast<WebOSShellSurface*>(resource->data);
    QString newAddon(path);
    if (that->m_addon != newAddon) {
        if (newAddon.isEmpty() || !that->m_surface->acceptsAddon(newAddon)) {
            that->setAddonStatus(WebOSSurfaceItem::AddonStatusDenied);
            return;
        }
        qDebug() << "addon changed" << that->m_addon << "to" << newAddon;
        that->m_addon = newAddon;
        emit that->addonChanged();
    }
}

void WebOSShellSurface::reset_addon(struct wl_client *client, struct wl_resource *resource)
{
    Q_UNUSED(client);
    WebOSShellSurface* that = static_cast<WebOSShellSurface*>(resource->data);
    that->m_addon.clear();
    emit that->addonChanged();
}
