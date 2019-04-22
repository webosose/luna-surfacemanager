// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef WAYLANDPRIMARYINPUTMETHOD_H
#define WAYLANDPRIMARYINPUTMETHOD_H

#include <QObject>
#include <QRect>
#include <QVector>

#include <wayland-server.h>

#include "waylandinputmethod.h"

class QQuickItem;
class QWaylandCompositor;
class WaylandTextModelFactory;
class WaylandInputPanelFactory;
class WaylandInputPanel;
class WaylandInputMethodManager;

/*!
 * Talks with the input method sitting in the VKB
 * process.
 */
class WaylandPrimaryInputMethod : public WaylandInputMethod {
    Q_OBJECT

    // For Qt5.6, Use QList rather than QVector.
    // Sequence types for qml-usage are more limited than Qt 5.12
    Q_PROPERTY(QList<QObject *> methods READ methods NOTIFY methodsChanged)

public:
    WaylandPrimaryInputMethod(QWaylandCompositor* compositor);
    ~WaylandPrimaryInputMethod();

    static void bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);

    void handleDestroy() Q_DECL_OVERRIDE;
    QList<QObject *> methods();
    WaylandInputMethod *getInputMethod(int displayId);

    void panelAdded(WaylandInputPanel *);

signals:
    void methodsChanged();

public slots:
    void insert();

private:
    WaylandTextModelFactory *m_factory;
    WaylandInputPanelFactory *m_panelFactory;

    QVector<QPointer<WaylandInputMethod>> m_methods;
};

#endif //WAYLANDPRIMARYINPUTMETHOD_H
