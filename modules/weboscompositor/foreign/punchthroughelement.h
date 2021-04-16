// Copyright (c) 2018-2021 LG Electronics, Inc.
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

#ifndef PUNCHTHROUGHELEMENT_H
#define PUNCHTHROUGHELEMENT_H

#include <qsgmaterial.h>
#include <qquickitem.h>
#include <qsgnode.h>
#include <qsgsimplerectnode.h>
#include <qsgflatcolormaterial.h>

class PunchThroughNode : public QSGGeometryNode
{
public:
    PunchThroughNode()
        : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    {
        setGeometry(&m_geometry);

        auto material = new QSGFlatColorMaterial();
        setMaterial(material);
        setFlag(OwnsMaterial);
    }

private:
    QSGGeometry m_geometry;
};

class PunchThroughItem : public QQuickItem
{
    Q_OBJECT
public:
    PunchThroughItem()
    {
        setFlag(ItemHasContents, true);
    }

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *) override
    {
        PunchThroughNode *n = static_cast<PunchThroughNode *>(node);
        if (!node)
            n = new PunchThroughNode();

        QSGGeometry::updateTexturedRectGeometry(n->geometry(), boundingRect(), QRectF(0, 0, 1, 1));
        auto material = static_cast<QSGFlatColorMaterial*>(n->material());
        material->setColor(m_color);
        material->setFlag(QSGMaterial::Blending, false);

        n->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

        return n;
    }

private:
    QColor m_color = Qt::transparent;
};
#endif // PUNCHTHROUGHELEMENT_H
