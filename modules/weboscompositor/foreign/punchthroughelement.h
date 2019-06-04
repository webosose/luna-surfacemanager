// Copyright (c) 2018-2019 LG Electronics, Inc.
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
#include <qsgsimplematerial.h>

struct State
{
    //Fragment color
    QColor color;
};

class PunchThroughShader : public QSGSimpleMaterialShader<State>
{
    QSG_DECLARE_SIMPLE_SHADER(PunchThroughShader, State);
public:
    const char *vertexShader() const {
        return
                "attribute highp vec4 vertex;                              \n"
                "uniform highp mat4 qt_Matrix;                             \n"
                "void main() {                                             \n"
                "    gl_Position = qt_Matrix * vertex;                     \n"
                "}";
    }

    const char *fragmentShader() const {
        return
                "uniform lowp vec4 color;                                  \n"
                "uniform lowp float qt_Opacity;                            \n"
                "void main ()                                              \n"
                "{                                                         \n"
                "    gl_FragColor = color * qt_Opacity;  \n"
                "}";
    }

    QList<QByteArray> attributes() const
    {
        return QList<QByteArray>() << "vertex";
    }

    void updateState(const State *state, const State *)
    {
        program()->setUniformValue(m_fragmentColor, state->color);
    }

    void resolveUniforms()
    {
        m_fragmentColor = program()->uniformLocation("color");
    }

private:
    int m_fragmentColor = 0;
};

class PunchThroughNode : public QSGGeometryNode
{
public:
    PunchThroughNode()
        : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    {
        setGeometry(&m_geometry);

        QSGSimpleMaterial<State> *material = PunchThroughShader::createMaterial();
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

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
    {
        PunchThroughNode *n = static_cast<PunchThroughNode *>(node);
        if (!node)
            n = new PunchThroughNode();

        QSGGeometry::updateTexturedRectGeometry(n->geometry(), boundingRect(), QRectF(0, 0, 1, 1));
        static_cast<QSGSimpleMaterial<State>*>(n->material())->state()->color = m_color;

        n->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

        return n;
    }

private:
    QColor m_color = Qt::transparent;
};
#endif // PUNCHTHROUGHELEMENT_H
