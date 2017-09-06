// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#include "webossurfacegroupcompositor.h"

#include "weboscorecompositor.h"
#include "webossurfaceitem.h"
#include "webossurfacegroup.h"

#include <QWaylandSurface>

#include <QtCompositor/private/qwlcompositor_p.h>
#include <QtCompositor/private/qwlsurface_p.h>

#include <QDebug>

#define WEBOSSURFACEGROUPCOMPOSITOR_VERSION 1

WebOSSurfaceGroupCompositor::WebOSSurfaceGroupCompositor(WebOSCoreCompositor* compositor)
    : QtWaylandServer::wl_webos_surface_group_compositor(compositor->handle()->wl_display(), WEBOSSURFACEGROUPCOMPOSITOR_VERSION)
    , m_compositor(compositor)
{
}

void WebOSSurfaceGroupCompositor::webos_surface_group_compositor_create_surface_group(Resource *resource, uint32_t id,
                                                             struct ::wl_resource *parent, const QString &name)
{
    if (m_groups.contains(name)) {
        wl_resource_post_error(resource->handle, WL_DISPLAY_ERROR_INVALID_OBJECT, "Group already exists");
        wl_resource_destroy(resource->handle);
        return;
    }

    QtWayland::Surface* surface = QtWayland::Surface::fromResource(parent);
    if (surface) {
        QWaylandQuickSurface *qsurface = qobject_cast<QWaylandQuickSurface *>(surface->waylandSurface());
        WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem*>(qsurface->surfaceItem());
        // A group can only be created to surfaces that
        // a) do not have a group
        // b) do not belong to a group
        if (item && !item->surfaceGroup()) {
            if (item->appId() != name) {
                qWarning("group name '%s' does not match the root surface appId '%s'", qPrintable(name), qPrintable(item->appId()));
            }
            qDebug("Creating surface group '%s' for wl_surface@%d", qPrintable(name), parent->object.id);
            WebOSSurfaceGroup* group = new WebOSSurfaceGroup;
            group->add(resource->client(), id, WEBOSSURFACEGROUP_VERSION);
            group->setName(name);
            group->setRootItem(item);
            group->setGroupCompositor(this);
            item->setSurfaceGroup(group);
            m_groups.insert(name, group);
        } else {
            wl_resource_post_error(resource->handle, WL_DISPLAY_ERROR_INVALID_OBJECT, "could not create group");
            wl_resource_destroy(resource->handle);
        }
    }
}

void WebOSSurfaceGroupCompositor::webos_surface_group_compositor_get_surface_group(Resource *resource, uint32_t id, const QString &name)
{
    if (m_groups.contains(name)) {
        qDebug("group '%s' found", qPrintable(name));
        WebOSSurfaceGroup* group = m_groups[name];
        group->add(resource->client(), id, WEBOSSURFACEGROUP_VERSION);
    } else {
        qWarning("group '%s' not found", qPrintable(name));
        // This group will try to attach immediatley
        // So make it as a dummy group to prevent client from crashed.
        WebOSSurfaceGroup* group = new WebOSSurfaceGroup;
        group->add(resource->client(), id, WEBOSSURFACEGROUP_VERSION);
        group->setGroupCompositor(this);
    }
}

void WebOSSurfaceGroupCompositor::removeGroup(WebOSSurfaceGroup* group)
{
    QString name = group->name();
    qDebug("Removing group '%s'", qPrintable(name));
    m_groups.remove(name);
}
