// Copyright (c) 2014-2022 LG Electronics, Inc.
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

#ifndef WEBOSSURFACEGROUP_H
#define WEBOSSURFACEGROUP_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <WebOSCoreCompositor/private/qwayland-server-webos-surface-group.h>

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include <QMap>

#define WEBOSSURFACEGROUP_VERSION 1

class WebOSSurfaceItem;
class WebOSSurfaceGroupCompositor;
class WebOSSurfaceGroupLayer;
class QQmlPropertyMap;

class WEBOS_COMPOSITOR_EXPORT WebOSSurfaceGroup : public QObject, public QtWaylandServer::wl_webos_surface_group {

    Q_OBJECT
    Q_PROPERTY(bool allowAnonymous READ allowAnonymous NOTIFY allowAnonymousChanged)
    Q_PROPERTY(WebOSSurfaceItem* rootItem READ rootItem NOTIFY rootItemChanged)
    Q_PROPERTY(WebOSSurfaceItem* keyboardFocusedSurface READ keyboardFocusedSurface NOTIFY keyboardFocusedSurfaceChanged)
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    Q_MOC_INCLUDE("webossurfaceitem.h")
#endif

public:
    WebOSSurfaceGroup();
    ~WebOSSurfaceGroup();

    bool allowAnonymous() { return m_allowAnonymous; }

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void setRootItem(WebOSSurfaceItem* item);
    WebOSSurfaceItem* rootItem() const { return m_root; }
    WebOSSurfaceItem* keyboardFocusedSurface() const { return m_keyboardFocusedSurface; }

    /*! A group might exists but without a root element the group is invalid. */
    bool isValid() { return m_root; }

    void setGroupCompositor(WebOSSurfaceGroupCompositor* gc) { m_groupCompositor = gc; }

    /*! Ment to be called from QML, do not share the raw pointer else where */
    Q_INVOKABLE QObject* layoutInfo(WebOSSurfaceItem* item) const;

    Q_INVOKABLE int keyIndex(WebOSSurfaceItem* item) const;

    void closeAttachedSurfaces();

    WebOSSurfaceItem* nextZOrderedSurfaceGroupItem(WebOSSurfaceItem* currentItem);

    Q_INVOKABLE bool allowLayerKeyOrder() const;
    void makeKeyOrderedItems();
    WebOSSurfaceItem* nextKeyOrderedSurfaceGroupItem(WebOSSurfaceItem* currentItem);

    WebOSSurfaceGroupLayer* surfaceGroupLayer(const QString &name) { return m_layers.value(name, nullptr); }

signals:
    void allowAnonymousChanged();
    void itemsChanged();
    void rootItemChanged();
    void keyboardFocusedSurfaceChanged();
    void keyOrderChanged();

protected:
    virtual void webos_surface_group_bind_resource(Resource *resource) override;

    virtual void webos_surface_group_destroy_resource(Resource *resource) override;
    virtual void webos_surface_group_destroy(Resource *resource) override;

    virtual void webos_surface_group_allow_anonymous_layers(Resource *resource, uint32_t allow) override;
    virtual void webos_surface_group_attach_anonymous(Resource *resource, struct ::wl_resource *surface, uint32_t z_hint) override;
    virtual void webos_surface_group_detach(Resource *resource, struct ::wl_resource *surface) override;

    virtual void webos_surface_group_create_layer(Resource *resource, uint32_t id, const QString &name, int32_t z_index) override;
    virtual void webos_surface_group_attach(Resource *resource, struct ::wl_resource *surface, const QString &layer_name) override;

    virtual void webos_surface_group_focus_owner(Resource *resource) override;
    virtual void webos_surface_group_focus_layer(Resource *resource, const QString &layer) override;

    virtual void webos_surface_group_commit_key_index(Resource *resource, uint32_t commit) override;

    void addZOrderedSurfaceLayoutInfoList(WebOSSurfaceItem* item, QSharedPointer<QObject> layoutInfo);
    void removeZOrderedSurfaceLayoutInfoList(WebOSSurfaceItem* item);
    QSharedPointer<QObject> layoutInfoFor(WebOSSurfaceItem* item) const;
    QSharedPointer<QObject> takeLayoutInfoFor(WebOSSurfaceItem* item);
    QList<WebOSSurfaceItem*> attachedClientSurfaceItems();

protected slots:
    void sortZOrderedSurfaceLayoutInfoList();

private:
    // methods
    bool assertOwner(Resource* resource);
    WebOSSurfaceItem* itemFromResource(struct ::wl_resource* surface);
    void removeFromGroup(WebOSSurfaceItem* item);
    void closeInvalidSurface(WebOSSurfaceItem* item);
    void removeAttachedItemsFromGroup();

private slots:
    void removeSurfaceItem();
    void removeLayer(const QString& name);
    void closeDeferredInvalidSurface();

private:
    // variables
    bool m_allowAnonymous;
    QString m_name;
    Resource* m_owner;
    WebOSSurfaceItem* m_root;
    WebOSSurfaceItem* m_keyboardFocusedSurface;
    WebOSSurfaceGroupCompositor* m_groupCompositor;
    QList<Resource *> m_resources;
    QMap<QString, WebOSSurfaceGroupLayer*> m_layers;
    int time;

    QList< QPair<WebOSSurfaceItem *, QSharedPointer<QObject> > > m_zOrderedSurfaceLayoutInfoList;
    QList<QPair<WebOSSurfaceItem *, int>> m_keyOrderedItems;
    bool m_useKeyIndex;
};
#endif
