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

#include <QDebug>

#include "webossurfacegrouplayer.h"
#include "webossurfacegroup.h"
#include "webossurfaceitem.h"

#define WEBOSSURFACEGROUPLAYER_VERSION 1

WebOSSurfaceGroupLayer::WebOSSurfaceGroupLayer(WebOSSurfaceGroup* group, struct ::wl_client *client, int id)
    : QtWaylandServer::wl_webos_surface_group_layer(client, id, WEBOSSURFACEGROUPLAYER_VERSION)
    , m_group(group)
    , m_attached(0)
    , m_keyIndex(0)
{
    m_layoutInfo.reset(new QObject);
    m_layoutInfo->setProperty("hint", false);
}

WebOSSurfaceGroupLayer::~WebOSSurfaceGroupLayer()
{
    qInfo("Deleting layer '%s'", qPrintable(m_name));
}

QSharedPointer<QObject> WebOSSurfaceGroupLayer::layoutInfo()
{
    return m_layoutInfo;
}

void WebOSSurfaceGroupLayer::webos_surface_group_layer_set_z_index(Resource *resource, int32_t z_index)
{
    Q_UNUSED(resource);
    setZ(z_index);
    qInfo() << "layer(" << m_name << ") z index :" << z() << "and key index(" << m_keyIndex << ") - attached:" << m_attached;
    if (m_attached) {
        emit m_attached->zOrderChanged(z());
    }
}

void WebOSSurfaceGroupLayer::webos_surface_group_layer_set_key_index(Resource *resource, int32_t key_index)
{
    Q_UNUSED(resource);
    setKeyIndex(key_index);
    qInfo() << "layer(" << m_name << ") key index :" << m_keyIndex << "at z(" << z() << ") - attached:" << m_attached;
    emit layerKeyIndexChanged(m_name);
}

void WebOSSurfaceGroupLayer::webos_surface_group_layer_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    emit layerDestroyed(m_name);
    detachSurface();
    delete this;
}

void WebOSSurfaceGroupLayer::webos_surface_group_layer_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void WebOSSurfaceGroupLayer::detachSurface()
{
    send_surface_detached();
    m_attached = NULL;
    qInfo() << "at layer " << m_name;
}

void WebOSSurfaceGroupLayer::attach(WebOSSurfaceItem* item)
{
    if (item) {
        m_attached = item;
        send_surface_attached();
        qInfo() << item << " attached at layer " << m_name;
    }
}
