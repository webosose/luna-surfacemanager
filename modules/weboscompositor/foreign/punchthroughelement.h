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
    QColor color;

    int compare(const State *other) const {
        uint rgb = color.rgba();
        uint otherRgb = other->color.rgba();

        if (rgb == otherRgb)
            return 0;
        else if (rgb < otherRgb)
            return -1;
        else
            return 1;
    }
};

class Shader : public QSGSimpleMaterialShader<State>
{
    QSG_DECLARE_SIMPLE_COMPARABLE_SHADER(Shader, State);
public:
    const char *vertexShader() const {
        return
                "attribute highp vec4 aVertex;                              \n"
                "attribute highp vec2 aTexCoord;                            \n"
                "uniform highp mat4 qt_Matrix;                              \n"
                "varying highp vec2 texCoord;                               \n"
                "void main() {                                              \n"
                "    gl_Position = qt_Matrix * aVertex;                     \n"
                "    texCoord = aTexCoord;                                  \n"
                "}";
    }

    const char *fragmentShader() const {
        return
                "uniform lowp float qt_Opacity;                             \n"
                "uniform lowp vec4 color;                                   \n"
                "varying highp vec2 texCoord;                               \n"
                "void main ()                                               \n"
                "{                                                          \n"
                "    gl_FragColor = color * qt_Opacity;  \n"
                "}";
    }

    QList<QByteArray> attributes() const
    {
        return QList<QByteArray>() << "aVertex" << "aTexCoord";
    }

    void updateState(const State *state, const State *)
    {
        program()->setUniformValue(id_color, state->color);
    }

    void resolveUniforms()
    {
        id_color = program()->uniformLocation("color");
    }

private:
    int id_color;
};

class ColorNode : public QSGGeometryNode
{
public:
    ColorNode()
        : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    {
        setGeometry(&m_geometry);

        QSGSimpleMaterial<State> *material = Shader::createMaterial();
        setMaterial(material);
        setFlag(OwnsMaterial);
    }

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
        ColorNode *n = static_cast<ColorNode *>(node);
        if (!node)
            n = new ColorNode();

        QSGGeometry::updateTexturedRectGeometry(n->geometry(), boundingRect(), QRectF(0, 0, 1, 1));
        static_cast<QSGSimpleMaterial<State>*>(n->material())->state()->color = m_color;

        n->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);

        return n;
    }

private:
    QColor m_color = Qt::transparent;
};

#endif // PUNCHTHROUGHELEMENT_H
