// Copyright (c) 2014-2019 LG Electronics, Inc.
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

#ifndef WEBOSSURFACEGROUPLAYER_H
#define WEBOSSURFACEGROUPLAYER_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <WebOSCoreCompositor/private/qwayland-server-webos-surface-group.h>

#include <QObject>
#include <QVariant>
#include <QSharedPointer>

class WebOSSurfaceGroup;
class WebOSSurfaceItem;

class WEBOS_COMPOSITOR_EXPORT WebOSSurfaceGroupLayer : public QObject, public QtWaylandServer::wl_webos_surface_group_layer {

    Q_OBJECT

public:
    WebOSSurfaceGroupLayer(WebOSSurfaceGroup* group, struct ::wl_client *client = 0, int id = 0);
    ~WebOSSurfaceGroupLayer();

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void setZ(int z) { m_layoutInfo->setProperty("z", z); }
    int z() { return m_layoutInfo->property("z").toInt(); }

    void setKeyIndex(int keyIndex) { m_keyIndex = keyIndex; }
    int keyIndex() { return m_keyIndex; }

    QSharedPointer<QObject> layoutInfo();

    bool attached() const { return m_attached; }
    WebOSSurfaceItem* attachedSurface() const { return m_attached; }
    void attach(WebOSSurfaceItem* item);

signals:
    void layerKeyIndexChanged(const QString& name);

public slots:
    void detachSurface();

signals:
    void layerDestroyed(const QString& name);

protected:
    virtual void webos_surface_group_layer_set_z_index(Resource *resource, int32_t z_index);
    virtual void webos_surface_group_layer_set_key_index(Resource *resource, int32_t key_index);
    virtual void webos_surface_group_layer_destroy_resource(Resource *resource);
    virtual void webos_surface_group_layer_destroy(Resource *resource);

private:
    QString m_name;
    WebOSSurfaceGroup* m_group;
    QSharedPointer<QObject> m_layoutInfo;
    WebOSSurfaceItem* m_attached;
    int m_keyIndex;
};
#endif
