// Copyright (c) 2013-2019 LG Electronics, Inc.
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

#ifndef COMPOSITOREXTENSION_H
#define COMPOSITOREXTENSION_H

#include <QtCompositor/qwaylandcompositor.h>
#include <QtCompositor/qwaylandsurface.h>
#include <compositorextensionglobal.h>

#include "compositorxinput.h"
#include "compositorxoutput.h"
#include "compositorxpointer.h"
#include "compositorwindowclose.h"

QT_BEGIN_NAMESPACE

class WEBOS_COMPOSITOR_EXT_EXPORT CompositorExtension : public QObject
{
    Q_OBJECT
public:
    CompositorExtension();
    virtual ~CompositorExtension() { }

    void setCompositor(QWaylandCompositor *compositor);
    QWaylandCompositor *compositor() { return m_compositor; }
    virtual void init() = 0;

    virtual CompositorXInput *xInput() { return 0; }
    virtual CompositorXOutput *xOutput() { return 0; }
    virtual CompositorXPointer *xPointer() { return 0; }
    virtual CompositorWindowClose *windowClose() { return 0; }

    virtual void handleHomeExposed() { }
    virtual void handleHomeUnexposed() { }

signals:
    void fullscreenSurfaceChanged(QWaylandSurface* oldSurface, QWaylandSurface* newSurface);
    void homeScreenExposed();
    void inputPanelRequested();
    void inputPanelDismissed();

protected:
    QWaylandCompositor *m_compositor;

private slots:
    void handleItemExposed(QString &);
    void handleItemUnexposed(QString &);
};

QT_END_NAMESPACE

#endif // COMPOSITOREXTENSION_H
