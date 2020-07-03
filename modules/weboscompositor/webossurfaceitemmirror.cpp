// Copyright (c) 2020 LG Electronics, Inc.
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

#include "weboscompositorwindow.h"
#include "webossurfaceitem.h"
#include "webossurfaceitemmirror.h"

WebOSSurfaceItemMirror::WebOSSurfaceItemMirror()
        : QQuickItem()
{
    qDebug() << "WebOSSurfaceItemMirror's constructor called";
}

WebOSSurfaceItemMirror::~WebOSSurfaceItemMirror()
{
    qDebug() << "WebOSSurfaceItemMirror's destructor called";

    setSourceItem(nullptr);
}

void WebOSSurfaceItemMirror::setSourceItem(WebOSSurfaceItem *sourceItem)
{
    qDebug() << "setSourceItem to replace" << m_sourceItem << "with" << sourceItem;

    if (m_sourceItem != sourceItem) {
        if (m_sourceItem) {
            disconnect(m_widthChangedConnection);
            disconnect(m_heightChangedConnection);
            disconnect(m_sourceDestroyedConnection);
            disconnect(m_mirrorDestroyedConnection);

            if (m_mirrorItem) {
                if (!m_sourceItem->removeMirrorItem(m_mirrorItem))
                    qCritical() << "Failed to remove mirror item";

                delete m_mirrorItem;
                m_mirrorItem = nullptr;
            }
        }

        if (sourceItem) {
            if (sourceItem->isMirrorItem() && sourceItem->mirrorSource()) {
                qDebug() << "Source item is already mirrored, use its mirror source" << sourceItem->mirrorSource();
                sourceItem = sourceItem->mirrorSource();
            }

            m_mirrorItem = sourceItem->createMirrorItem();
            if (m_mirrorItem) {
                m_mirrorItem->setParentItem(this);
                m_mirrorItem->setSizeFollowsSurface(false);
                m_mirrorItem->setSize(size());

                m_widthChangedConnection = connect(this, &QQuickItem::widthChanged, [this]() {
                    if (m_mirrorItem)
                        m_mirrorItem->setWidth(width());
                });
                m_heightChangedConnection = connect(this, &QQuickItem::heightChanged, [this]() {
                    if (m_mirrorItem)
                        m_mirrorItem->setHeight(height());
                });
                m_sourceDestroyedConnection = connect(sourceItem, &WebOSSurfaceItem::surfaceAboutToBeDestroyed, [this]() {
                    qDebug() << "Source(" << m_sourceItem << ")'s surface is about to be destroyed";
                    setSourceItem(nullptr);
                });
                m_mirrorDestroyedConnection = connect(m_mirrorItem, &WebOSSurfaceItem::surfaceAboutToBeDestroyed, [this]() {
                    qDebug() << "Mirror(" << m_mirrorItem << ")'s surface is about to be destroyed";
                    m_mirrorItem = nullptr;
                });
            } else {
                qWarning() << "Failed to start mirroring for" << sourceItem;
                return;
            }
        }

        m_sourceItem = sourceItem;
        emit sourceItemChanged();
    }
}
