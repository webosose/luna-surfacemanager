// Copyright (c) 2014-2021 LG Electronics, Inc.
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

#include "webossurfacegroup.h"
#include "webossurfacegroupcompositor.h"
#include "webossurfacegrouplayer.h"
#include "webossurfaceitem.h"
#include "webosgroupedwindowmodel.h"
#include "weboscorecompositor.h"

#include <QtWaylandCompositor/private/qwaylandsurface_p.h>
#include <QDebug>
#include <QQmlPropertyMap>
#include <QQmlEngine>
#include <algorithm>

WebOSSurfaceGroup::WebOSSurfaceGroup()
    : QtWaylandServer::wl_webos_surface_group()
    , m_allowAnonymous(false)
    , m_owner(0)
    , m_root(0)
    , m_keyboardFocusedSurface(0)
    , m_groupCompositor(0)
    , time(0)
    , m_useKeyIndex(false)
{
}

WebOSSurfaceGroup::~WebOSSurfaceGroup()
{
    qInfo("deleting group '%s'", qPrintable(m_name));
    removeAttachedItemsFromGroup();
    m_layers.clear();
    m_zOrderedSurfaceLayoutInfoList.clear();
    m_keyOrderedItems.clear();
}

void WebOSSurfaceGroup::webos_surface_group_bind_resource(Resource *resource)
{
    if (!m_owner) {
        m_owner = resource;
    } else {
        m_resources << resource;
    }
}

void WebOSSurfaceGroup::webos_surface_group_destroy_resource(Resource *resource)
{
    if (resource == m_owner) {
        qInfo("Group '%s' owner destroyed", qPrintable(m_name));
        if (m_groupCompositor)
            m_groupCompositor->removeGroup(this);
        if (m_root)
            m_root->setSurfaceGroup(NULL);

        m_owner = NULL;
        m_root = NULL;
    } else {
        m_resources.removeOne(resource);
    }

    // The group will get deleted when the last client destroys its associated
    // resource to this group
    if (m_resources.isEmpty() && !m_owner) {
        delete this;
    } else if (!m_owner) {
        qWarning("Group '%s' owner destroyed, has attached clients", qPrintable(m_name));
        foreach (Resource* r, m_resources) {
            qInfo("Informing group '%s' wl_resource@%d", qPrintable(m_name), r->handle->object.id);
            send_owner_destroyed(r->handle);
        }
    }
}

void WebOSSurfaceGroup::webos_surface_group_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void WebOSSurfaceGroup::webos_surface_group_allow_anonymous_layers(Resource *resource, uint32_t allow)
{
    if (!assertOwner(resource)) {
        return;
    }

    if (m_allowAnonymous != allow) {
        m_allowAnonymous = allow;
        emit allowAnonymousChanged();
    }
}

void WebOSSurfaceGroup::webos_surface_group_attach_anonymous(Resource *resource, struct ::wl_resource *surface, uint32_t z_hint)
{
    Q_UNUSED(resource);
    if (m_allowAnonymous) {
        WebOSSurfaceItem* item = itemFromResource(surface);
        if (item) {
            static_cast<QWaylandQuickSurface *>(item->surface())->setUseTextureAlpha(true);
            qInfo("Attaching anonymous wl_surface@%d, to group '%s'", surface->object.id, qPrintable(m_name));
            connect(item, SIGNAL(itemAboutToBeDestroyed()), this, SLOT(removeSurfaceItem()));
            QSharedPointer<QObject> li = QSharedPointer<QObject>(new QObject);
            li->setProperty("z", z_hint);
            li->setProperty("hint", true);
            li->setProperty("time", time++);
            addZOrderedSurfaceLayoutInfoList(item, li);
            item->setSurfaceGroup(this);
            emit itemsChanged();
        } else {
            qWarning("Could not find WebOSSurfaceItem for wl_surface@%d", surface->object.id);
        }
    } else {
        qWarning("Trying to attach to group '%s', which does not allow anonymous layers", qPrintable(m_name));
    }
}

void WebOSSurfaceGroup::webos_surface_group_detach(Resource *resource, struct ::wl_resource *surface)
{
    if (m_owner != resource) {
        qInfo("wl_surface@%d detaching from group '%s'", surface->object.id, qPrintable(m_name));
        removeFromGroup(itemFromResource(surface));
    } else {
        qWarning("Detaching of root wl_surface@%d from group '%s' not allowed", surface->object.id, qPrintable(m_name));
    }
}


void WebOSSurfaceGroup::webos_surface_group_create_layer(Resource *resource, uint32_t id, const QString &name, int32_t z_index)
{
    QList<WebOSSurfaceGroupLayer*> layers = m_layers.values();
    foreach (WebOSSurfaceGroupLayer* l, layers) {
        if (l->name() == name) {
            wl_resource_post_error(resource->handle, WL_DISPLAY_ERROR_INVALID_OBJECT, "Layer name already exists");
            wl_resource_destroy(resource->handle);
            return;
        }

        if (l->z() == z_index) {
            wl_resource_post_error(resource->handle, WL_DISPLAY_ERROR_INVALID_OBJECT, "Layer z index already exists");
            wl_resource_destroy(resource->handle);
            return;
        }
    }

    qInfo("Creating layer '%s':%d to group '%s'", qPrintable(name), z_index, qPrintable(m_name));
    WebOSSurfaceGroupLayer* newLayer = new WebOSSurfaceGroupLayer(this, resource->client(), id);
    newLayer->setName(name);
    newLayer->setZ(z_index);

    connect(newLayer, SIGNAL(layerDestroyed(const QString&)), this, SLOT(removeLayer(const QString&)));
    m_layers.insert(name, newLayer);
}

void WebOSSurfaceGroup::webos_surface_group_attach(Resource *resource, struct ::wl_resource *surface, const QString &layer_name)
{
    WebOSSurfaceItem* item = itemFromResource(surface);
    if (!item) {
        qWarning("Could not find WebOSSurfaceItem for wl_surface@%d", surface->object.id);
        return;
    }

    if (m_layers.contains(layer_name)) {
        WebOSSurfaceGroupLayer* l = m_layers[layer_name];
        if (l->attached()) {
            qWarning("Layer '%s:%s' already attached", qPrintable(m_name), qPrintable(layer_name));
            wl_resource_post_error(resource->handle, WL_DISPLAY_ERROR_INVALID_OBJECT, "Layer already attached");
            wl_resource_destroy(resource->handle);
        } else {
            static_cast<QWaylandQuickSurface *>(item->surface())->setUseTextureAlpha(true);
            qInfo("Attaching wl_surface@%d to %s:%s", surface->object.id, qPrintable(m_name), qPrintable(layer_name));
            connect(item, SIGNAL(itemAboutToBeDestroyed()), this, SLOT(removeSurfaceItem()));
            addZOrderedSurfaceLayoutInfoList(item, l->layoutInfo());
            l->attach(item);
            item->setSurfaceGroup(this);
            makeKeyOrderedItems();
        }
    } else {
        qWarning("Layer '%s' does not exist in group '%s'", qPrintable(layer_name), qPrintable(m_name));
        closeInvalidSurface(item);
    }
}

QObject* WebOSSurfaceGroup::layoutInfo(WebOSSurfaceItem* item) const
{
    QSharedPointer<QObject> p = layoutInfoFor(item);
    QQmlPropertyMap* map = new QQmlPropertyMap;
    QQmlEngine::setObjectOwnership(map, QQmlEngine::JavaScriptOwnership);
    if (!p.isNull()) {
        map->insert(QLatin1String("z"), p->property("z"));
        map->insert(QLatin1String("hint"), p->property("hint"));
        map->insert(QLatin1String("time"), p->property("time"));
    }
    return map;
}

int WebOSSurfaceGroup::keyIndex(WebOSSurfaceItem* item) const
{
    foreach(WebOSSurfaceGroupLayer* l, m_layers.values()) {
        if (l->attachedSurface() == item) {
            return l->keyIndex();
        }
    }
    return 0;
}

bool WebOSSurfaceGroup::assertOwner(Resource* r)
{
    if (m_owner != r) {
        wl_resource_post_error(r->handle, WL_DISPLAY_ERROR_INVALID_METHOD, "Trying to alter non-owned surface group");
        wl_resource_destroy(r->handle);
        return false;
    }
    return true;
}

WebOSSurfaceItem* WebOSSurfaceGroup::itemFromResource(struct ::wl_resource* surface)
{
    QWaylandSurface* s = QWaylandSurface::fromResource(surface);
    if (s) {
        QWaylandQuickSurface *qsurface = qobject_cast<QWaylandQuickSurface *>(s);
        return WebOSSurfaceItem::getSurfaceItemFromSurface(qsurface);
    }
    qWarning("Could not resolve wl_surface@%d to surface item", surface->object.id);
    return NULL;
}

void WebOSSurfaceGroup::removeFromGroup(WebOSSurfaceItem* item)
{
    if (!item) {
        qWarning() << "removeFromGroup(): input item is nullptr, name=" << qPrintable(m_name);
        return;
    }
    qInfo() << "Removed " << item << "from group ", qPrintable(m_name);

    item->disconnect(this);
    QSharedPointer<QObject> li = takeLayoutInfoFor(item);
    li.clear();

    // Remove the surface from a layer
    foreach(WebOSSurfaceGroupLayer* l, m_layers.values()) {
        if (l->attachedSurface() == item) {
            l->detachSurface();
            WebOSGroupedWindowModel* groupedModel = item->groupedWindowModel();
            if (groupedModel) {
                disconnect(item, SIGNAL(zOrderChanged(int)), groupedModel, SLOT(handleZOrderChange()));
            }

            /* To be in recent model */
            if (m_groupCompositor && m_groupCompositor->compositor()) {
                item->setLastFullscreenTick(m_groupCompositor->compositor()->getFullscreenTick());
            }
        }
    }

    // This will trigger the re-evaluation of the windowmodel
    item->setSurfaceGroup(NULL);

    makeKeyOrderedItems();

    if (m_keyboardFocusedSurface == item) {
        if (m_root) {
            m_keyboardFocusedSurface = m_root;
            emit keyboardFocusedSurfaceChanged();
        }
    }
}

void WebOSSurfaceGroup::closeInvalidSurface(WebOSSurfaceItem* item)
{
    if (!item) {
        qWarning() << "Invalid Input WebOSSurfaceItem";
        return;
    }

    if (item->appId().isEmpty()) {
        qInfo() << item <<  "is empty, we are waiting for appId";
        connect(item, SIGNAL(appIdChanged()), this, SLOT(closeDeferredInvalidSurface()));
        return;
    }

    //Send close instead of wl_resource_post_error to keep client
    if (m_groupCompositor && m_groupCompositor->compositor()) {
        m_groupCompositor->compositor()->closeWindow(QVariant::fromValue(item));
    }
}

void WebOSSurfaceGroup::closeDeferredInvalidSurface()
{
    WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem *>(sender());

    if (item->appId().isEmpty()) {
        qInfo() << item <<  "is empty, we are waiting for appId";
        return;
    }

    //Send close instead of wl_resource_post_error to keep client
    if (m_groupCompositor && m_groupCompositor->compositor()) {
        m_groupCompositor->compositor()->closeWindow(QVariant::fromValue(item));
    }
    disconnect(item, SIGNAL(appIdChanged()), this, SLOT(closeDeferredInvalidSurface()));
}


void WebOSSurfaceGroup::removeSurfaceItem()
{
    WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem *>(sender());
    removeFromGroup(item);
}

void WebOSSurfaceGroup::removeLayer(const QString& name)
{
    qInfo("Removing layer '%s' for group '%s'", qPrintable(name), qPrintable(m_name));
    m_layers.take(name);
    makeKeyOrderedItems();
}

void WebOSSurfaceGroup::removeAttachedItemsFromGroup()
{
    QList<WebOSSurfaceItem*> attachedItems = attachedClientSurfaceItems();
    foreach (WebOSSurfaceItem* i, attachedItems)
       removeFromGroup(i);
}

void WebOSSurfaceGroup::closeAttachedSurfaces()
{
    QList<WebOSSurfaceItem*> attachedItems = attachedClientSurfaceItems();
    foreach (WebOSSurfaceItem* i, attachedItems) {
        i->setItemState(WebOSSurfaceItem::ItemStateClosing);
        if (m_groupCompositor && m_groupCompositor->compositor()) {
            m_groupCompositor->compositor()->closeWindow(QVariant::fromValue(i));
        }
    }
}

void WebOSSurfaceGroup::webos_surface_group_focus_owner(Resource *resource)
{
    Q_UNUSED(resource);
    if (m_root) {
        qInfo() << "surface group(" << m_name << ")";
        m_keyboardFocusedSurface = m_root;
        emit keyboardFocusedSurfaceChanged();
    }
}

void WebOSSurfaceGroup::webos_surface_group_focus_layer(Resource *resource, const QString &layer)
{
    Q_UNUSED(resource);
    if (m_layers.contains(layer)) {
        WebOSSurfaceGroupLayer *surfaceGroupLayer = m_layers[layer];
        if (surfaceGroupLayer) {
            WebOSSurfaceItem* surfaceItem = surfaceGroupLayer->attachedSurface();
            if (surfaceItem) {
                qInfo() << "surface group(" << m_name << ") layer:" << layer;
                m_keyboardFocusedSurface = surfaceItem;
                emit keyboardFocusedSurfaceChanged();
            }
        }
    }
}


void WebOSSurfaceGroup::webos_surface_group_commit_key_index(Resource *resource, uint32_t commit)
{
    Q_UNUSED(resource);
    qInfo() << "surface group(" << m_name << ") commit:" << commit;

    if (commit)
        makeKeyOrderedItems();

    m_useKeyIndex = commit;
    emit keyOrderChanged();
}

void WebOSSurfaceGroup::setRootItem(WebOSSurfaceItem* item)
{
    qInfo() << "surface group(" << m_name << ") set root:" << item;
    if (m_root != item) {
        m_root = item;
        if (m_root) {
            QSharedPointer<QObject> li = QSharedPointer<QObject>(new QObject);
            addZOrderedSurfaceLayoutInfoList(m_root, li);
            makeKeyOrderedItems();
        } else {
            m_zOrderedSurfaceLayoutInfoList.clear();
            m_keyOrderedItems.clear();
        }
    }
}

bool zOrderedLessThan(const QPair<WebOSSurfaceItem *, QSharedPointer<QObject> > pair1, const QPair<WebOSSurfaceItem *, QSharedPointer<QObject> > pair2)
{
    return pair1.first->z() <  pair2.first->z();
}

WebOSSurfaceItem* WebOSSurfaceGroup::nextZOrderedSurfaceGroupItem(WebOSSurfaceItem* currentItem)
{
    WebOSSurfaceItem* returnItem = NULL;
    if (currentItem) {
        if (!m_zOrderedSurfaceLayoutInfoList.isEmpty()) {
            for (int i = (m_zOrderedSurfaceLayoutInfoList.size() - 1) ; i >= 0 ; --i) {
                if (m_zOrderedSurfaceLayoutInfoList[i].first == currentItem) {
                    if (i > 0) {
                        returnItem = qobject_cast<WebOSSurfaceItem *>(m_zOrderedSurfaceLayoutInfoList[i - 1].first);
                        break;
                    }
                }
            }
        }
    }
    return returnItem;
}

void WebOSSurfaceGroup::addZOrderedSurfaceLayoutInfoList(WebOSSurfaceItem* item, QSharedPointer<QObject> layoutInfo)
{
    if (item && layoutInfo) {

        m_zOrderedSurfaceLayoutInfoList.append(qMakePair(item, layoutInfo));

        sortZOrderedSurfaceLayoutInfoList();
        connect(item, SIGNAL(zChanged()), this, SLOT(sortZOrderedSurfaceLayoutInfoList()));
    }
}

void WebOSSurfaceGroup::removeZOrderedSurfaceLayoutInfoList(WebOSSurfaceItem* item)
{
    if (item && !m_zOrderedSurfaceLayoutInfoList.isEmpty()) {
        for (int i = (m_zOrderedSurfaceLayoutInfoList.size() - 1) ; i >= 0 ; --i) {
            if (m_zOrderedSurfaceLayoutInfoList[i].first == item) {
                m_zOrderedSurfaceLayoutInfoList.removeAll(m_zOrderedSurfaceLayoutInfoList[i]);
                disconnect(item, SIGNAL(zChanged()), this, SLOT(sortZOrderedSurfaceLayoutInfoList()));
                sortZOrderedSurfaceLayoutInfoList();
                break;
            }
        }
    }
}

QSharedPointer<QObject> WebOSSurfaceGroup::layoutInfoFor(WebOSSurfaceItem* item) const
{
    QSharedPointer<QObject> returnvalue;
    if (item && !m_zOrderedSurfaceLayoutInfoList.isEmpty()) {
        for (int i = (m_zOrderedSurfaceLayoutInfoList.size() - 1) ; i >= 0 ; --i) {
            if (m_zOrderedSurfaceLayoutInfoList[i].first == item) {
                returnvalue = m_zOrderedSurfaceLayoutInfoList[i].second;
                break;
            }
        }
    }
    return returnvalue;
}

QSharedPointer<QObject> WebOSSurfaceGroup::takeLayoutInfoFor(WebOSSurfaceItem* item)
{
    QSharedPointer<QObject> returnvalue;
    if (item && !m_zOrderedSurfaceLayoutInfoList.isEmpty()) {
        for (int i = (m_zOrderedSurfaceLayoutInfoList.size() - 1) ; i >= 0 ; --i) {
            if (m_zOrderedSurfaceLayoutInfoList[i].first == item) {
                returnvalue = m_zOrderedSurfaceLayoutInfoList[i].second;
                m_zOrderedSurfaceLayoutInfoList.removeAll(m_zOrderedSurfaceLayoutInfoList[i]);
                disconnect(item, SIGNAL(zChanged()), this, SLOT(sortZOrderedSurfaceLayoutInfoList()));
                sortZOrderedSurfaceLayoutInfoList();
                break;
            }
        }
    }
    return returnvalue;
}

QList<WebOSSurfaceItem*> WebOSSurfaceGroup::attachedClientSurfaceItems()
{
    QList<WebOSSurfaceItem*> returnvalue;
    if (!m_zOrderedSurfaceLayoutInfoList.isEmpty()) {
        for (int i = (m_zOrderedSurfaceLayoutInfoList.size() - 1) ; i >= 0 ; --i) {
            if (m_zOrderedSurfaceLayoutInfoList[i].first != m_root) {
                returnvalue.append(m_zOrderedSurfaceLayoutInfoList[i].first);
            }
        }
    }
    return returnvalue;
}

void WebOSSurfaceGroup::sortZOrderedSurfaceLayoutInfoList()
{
    if (!m_zOrderedSurfaceLayoutInfoList.isEmpty())
        std::sort(m_zOrderedSurfaceLayoutInfoList.begin(), m_zOrderedSurfaceLayoutInfoList.end(), zOrderedLessThan);
}

bool WebOSSurfaceGroup::allowLayerKeyOrder() const
{
    if (m_allowAnonymous || !m_useKeyIndex || m_keyOrderedItems.isEmpty())
        return false;
    return true;
}

bool keyOrderedLessThan(const QPair<WebOSSurfaceItem *, int> pair1, const QPair<WebOSSurfaceItem *, int> pair2)
{
    return pair1.second <  pair2.second;
}

void WebOSSurfaceGroup::makeKeyOrderedItems()
{
    m_keyOrderedItems.clear();

    m_keyOrderedItems.append(qMakePair(m_root, 0));
    foreach (WebOSSurfaceGroupLayer* l, m_layers.values()) {
        if (l->keyIndex() && l->attachedSurface())
            m_keyOrderedItems.append(qMakePair(l->attachedSurface(), l->keyIndex()));
    }
    if (!m_keyOrderedItems.isEmpty()) {
        std::sort(m_keyOrderedItems.begin(), m_keyOrderedItems.end(), keyOrderedLessThan);
    }
}

WebOSSurfaceItem* WebOSSurfaceGroup::nextKeyOrderedSurfaceGroupItem(WebOSSurfaceItem* currentItem)
{
    WebOSSurfaceItem* returnItem = NULL;
    int currentItemIndex = m_keyOrderedItems.size() - 1;

    if (currentItem) {
        if (!m_keyOrderedItems.isEmpty()) {
            for (int i = (m_keyOrderedItems.size() - 1) ; i >= 0 ; --i) {
                if (m_keyOrderedItems[i].first == currentItem) {
                    currentItemIndex = i;
                    break;
                }
            }
        }
    }

    if (currentItemIndex > 0) {
        for (int i = currentItemIndex ; i >= 0 ; --i) {
            if (i > 0 && m_keyOrderedItems[i - 1].first) {
                returnItem = qobject_cast<WebOSSurfaceItem *>(m_keyOrderedItems[i - 1].first);
                break;
            }
        }
    }
    return returnItem;
}

WebOSSurfaceItem* WebOSSurfaceGroup::findKeyFocusedItem()
{
    WebOSSurfaceItem* returnItem = NULL;
    int topIndex = -1;

    if (allowLayerKeyOrder()) {
        topIndex = m_keyOrderedItems.size() -1;
        qInfo() << "topIndex: " << topIndex << " in m_keyOrderedItems: " << m_keyOrderedItems.size();
        if (topIndex >= 0)
            for (int i = topIndex; i >=0; i--) {
                WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem *>(m_keyOrderedItems[i].first);
                if (item && item->isMapped()) {
                    returnItem = item;
                    break;
                } else {
                    qWarning() << "[GROUP]" << item << " is not mapped. Find next key order item";
                }
            }
    } else {
        if (!m_zOrderedSurfaceLayoutInfoList.isEmpty()) {
            topIndex = m_zOrderedSurfaceLayoutInfoList.size() - 1;
            qInfo() << "topIndex: " << topIndex << " in m_zOrderedSurfaceLayoutInfoList: " << m_zOrderedSurfaceLayoutInfoList.size();
            if (topIndex >= 0)
                for (int i = topIndex; i >=0; i--) {
                    WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem *>(m_zOrderedSurfaceLayoutInfoList[i].first);
                    if (item && item->isMapped()) {
                        returnItem = item;
                        break;
                    } else {
                        qWarning() << "[GROUP]" << item << "is not mapped. Find next z ordered surface item";
                    }
                }
        }
    }

    if (returnItem)
        qInfo() << "returnItem: " << returnItem;
    else
        qInfo() << "returnItem is null";

    return returnItem;
}
