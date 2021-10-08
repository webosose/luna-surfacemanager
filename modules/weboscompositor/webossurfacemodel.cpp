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

#include "webossurfacemodel.h"
#include "webossurfaceitem.h"
#include <QDebug>
#include "weboscompositortracer.h"

WebOSSurfaceModel::WebOSSurfaceModel(QObject *parent)
    : m_dataDirty(false)
    , m_firstDirtyIndex(0)
    , m_lastDirtyIndex(0)
{
    Q_UNUSED(parent)
    roles[0] = "surfaceItem";
    // we handle the dataChanged in this class but wait after we come back from the event loop.
    // This allows us to collect item updates from one function into a single model update
    // FIXME: Disabling defered dataChanged signal as it breaks the surface logic in QML side
    // connect(this, SIGNAL(deferDataChanged()), this, SLOT(handleDeferDataChanged()), Qt::QueuedConnection);
    connect(this, SIGNAL(deferDataChanged()), this, SLOT(handleDeferDataChanged()), Qt::DirectConnection);
}

WebOSSurfaceModel::~WebOSSurfaceModel()
{
    clear();
}

QHash<int, QByteArray> WebOSSurfaceModel::roleNames() const
{
    return roles;
}

int WebOSSurfaceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_list.size();
}

QVariant WebOSSurfaceModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    if (index.row() < 0 || index.row() >= m_list.size())
        return QVariant();

    // surfaceItem is our only role for now. we might have something like a title, enabled state etc here.
    // just returns the corresponding surfaceitem
    return QVariant::fromValue(m_list.at(index.row()));
}

void WebOSSurfaceModel::appendRow(WebOSSurfaceItem *item)
{
    PMTRACE_FUNCTION;
    appendRows(QList<WebOSSurfaceItem*>() << item);
}

void WebOSSurfaceModel::appendRows(const QList<WebOSSurfaceItem *> &items)
{
    PMTRACE_FUNCTION;
    beginInsertRows(QModelIndex(), rowCount(), rowCount() + items.size() - 1);
    foreach (WebOSSurfaceItem *item, items) {
        connect(item, SIGNAL(dataChanged()), SLOT(handleItemChange()));
        connect(item, SIGNAL(windowClassChanged()), SLOT(handleItemChange()));
        m_list.append(item);
    }
    endInsertRows();
}

void WebOSSurfaceModel::insertRow(int row, WebOSSurfaceItem *item)
{
    PMTRACE_FUNCTION;
    beginInsertRows(QModelIndex(), row, row);
    connect(item, SIGNAL(fullscreenChanged(bool)), SLOT(handleItemChange()));
    m_list.insert(row, item);
    endInsertRows();
}

bool WebOSSurfaceModel::removeRow(int row, const QModelIndex &parent)
{
    PMTRACE_FUNCTION;
    Q_UNUSED(parent);
    if (row < 0 || row >= m_list.size())
        return false;
    beginRemoveRows(QModelIndex(), row, row);
    delete m_list.takeAt(row);
    endRemoveRows();
    return true;
}

bool WebOSSurfaceModel::removeRows(int row, int count, const QModelIndex &parent)
{
    PMTRACE_FUNCTION;
    Q_UNUSED(parent);
    if (row < 0 || (row + count) >= m_list.size())
        return false;
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i)
        delete m_list.takeAt(row);
    endRemoveRows();
    return true;
}

WebOSSurfaceItem* WebOSSurfaceModel::takeRow(int row)
{
    PMTRACE_FUNCTION;
    beginRemoveRows(QModelIndex(), row, row);
    WebOSSurfaceItem* item = m_list.takeAt(row);
    endRemoveRows();
    return item;
}

QModelIndex WebOSSurfaceModel::indexFromItem(const WebOSSurfaceItem *item) const
{
    PMTRACE_FUNCTION;
    Q_ASSERT(item);
    for (int row = 0; row < m_list.size(); ++row) {
        if (m_list.at(row) == item)
            return index(row);
    }
    return QModelIndex();
}

WebOSSurfaceItem* WebOSSurfaceModel::surfaceItemForAppId(const QString& appId)
{
    PMTRACE_FUNCTION;
    for (int row = 0; row < m_list.size(); ++row) {
        WebOSSurfaceItem* item = static_cast<WebOSSurfaceItem*>(m_list.at(row));
        if (item->appId() == appId)
            return item;
    }
    return NULL;
}

WebOSSurfaceItem* WebOSSurfaceModel::surfaceItemForIndex(int index)
{
    PMTRACE_FUNCTION;
    if (index < 0 || index >= m_list.size())
        return NULL;
    WebOSSurfaceItem* item = static_cast<WebOSSurfaceItem*>(m_list.at(index));
    return item;
}

WebOSSurfaceItem* WebOSSurfaceModel::getLastRecentItem()
{
    PMTRACE_FUNCTION;
    for (int row = 0; row < m_list.size(); ++row) {
        WebOSSurfaceItem* item = static_cast<WebOSSurfaceItem*>(m_list.at(row));
        QVariantMap windowProperties = item->windowProperties();
        QString surfaceType = windowProperties.value(QLatin1String("_WEBOS_WINDOW_TYPE")).toString();
        if ((surfaceType.isEmpty() || // This is for plain Qt apps that do not set window type.
             surfaceType == QLatin1String("_WEBOS_WINDOW_TYPE_CARD")))
            return item;
    }
    return NULL;
}

void WebOSSurfaceModel::clear()
{
    qDeleteAll(m_list);
    m_list.clear();
}

void WebOSSurfaceModel::handleItemChange()
{
    PMTRACE_FUNCTION;
    WebOSSurfaceItem* item = static_cast<WebOSSurfaceItem*>(sender());
    QModelIndex index = indexFromItem(item);
    if (index.isValid()) {
        if (m_dataDirty) {
            if (index.row() < m_firstDirtyIndex)
                m_firstDirtyIndex = index.row();
            else if (index.row() > m_lastDirtyIndex)
                m_lastDirtyIndex = index.row();
        } else {
            m_dataDirty = true;
            m_firstDirtyIndex = index.row();
            m_lastDirtyIndex = index.row();
            emit deferDataChanged();
        }
    }
}

void WebOSSurfaceModel::handleDeferDataChanged()
{
    PMTRACE_FUNCTION;
    m_dataDirty = false;
    // TODO: Optimize the notifying range. Right now we emit the whole range, since
    // there are transients "somewhere" and the filter proxies need to traverse all
    // surfaces in order to find the right items
    // FIXME transients may have been broken. we just emit a block of changed items
    emit dataChanged(createIndex(m_firstDirtyIndex, 0), createIndex(m_lastDirtyIndex, 0));
    m_firstDirtyIndex = m_lastDirtyIndex = 0;
}

void WebOSSurfaceModel::surfaceUnmapped(WebOSSurfaceItem *item)
{
    PMTRACE_FUNCTION;
    Q_ASSERT(item);
    QModelIndex index = indexFromItem(item);
    if (index.isValid())
        takeRow(index.row());
}

void WebOSSurfaceModel::surfaceMapped(WebOSSurfaceItem *item)
{
    PMTRACE_FUNCTION;
    Q_ASSERT(item);
    QModelIndex index = indexFromItem(item);
    if (!index.isValid())
        appendRow(item);
    else
        qWarning() << "Item already exists in the model" << item;
}

void WebOSSurfaceModel::surfaceDestroyed(WebOSSurfaceItem *item)
{
    PMTRACE_FUNCTION;
    Q_ASSERT(item);
    QModelIndex index = indexFromItem(item);
    if (index.isValid())
        takeRow(index.row());
}
