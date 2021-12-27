// Copyright (c) 2019-2022 LG Electronics, Inc.
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

#include <QDebug>
#include <QWaylandCompositor>

#include <wayland-input-method-server-protocol.h>

#include "waylandinputmethod.h"
#include "waylandinputpanel.h"
#include "waylandprimaryinputmethod.h"
#include "waylandtextmodelfactory.h"
#include "waylandinputpanelfactory.h"
#include "waylandinputmethodmanager.h"

WaylandPrimaryInputMethod::WaylandPrimaryInputMethod(QWaylandCompositor* compositor)
    : WaylandInputMethod(compositor)
{
}

void WaylandPrimaryInputMethod::initialize()
{
    WaylandInputMethod::initialize();
    m_factory = new WaylandTextModelFactory(compositor(), this);
    m_panelFactory = new WaylandInputPanelFactory(compositor(), this);

    // The PrimaryInputMethod works like a factory for input_method
    wl_display_add_global(compositor()->display(), &input_method_interface, this, WaylandPrimaryInputMethod::bind);
}

WaylandPrimaryInputMethod::~WaylandPrimaryInputMethod()
{
    delete m_factory;
    delete m_panelFactory;
}

void WaylandPrimaryInputMethod::insert()
{
    WaylandInputMethod *method = qobject_cast<WaylandInputMethod *>(sender());

    if (!method || method->displayId() < 0) {
        qWarning() << "Cannot insert method with invalid sender or displayId";
        return;
    }

    int sizeNeeded = method->displayId() + 1;
    int displayId = method->displayId();

    if (sizeNeeded > m_methods.size())
        m_methods.resize(sizeNeeded);

    if (m_methods[displayId]) {
        qWarning() << "Method already exists for display" << m_methods[displayId] << displayId;
        return;
    }

    qInfo() << method << "inserted at" << displayId;
    m_methods[displayId] = method;
    emit methodsChanged();
}

void WaylandPrimaryInputMethod::bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    if (version >= 2) {
        WaylandPrimaryInputMethod* primary = static_cast<WaylandPrimaryInputMethod*>(data);
        WaylandInputMethod *method = primary;

        // NOTE: The PrimaryInputMethod has the first bound input_method.
        // But it is not guaranteed that the input_method has display ID '0'.
        // It depends on the order of 'boot up' for the MaliitServer instances.
        //Namely, the PrimaryInputMethod can be bound to any display ID.
        if (primary->handle()) {
            qDebug() << "Primary is already bound, trying to bind secondary one";
            method = new WaylandInputMethod(primary->compositor());
        }

        method->binding(client, id);
        connect(method, &WaylandInputMethod::displayIdChanged, primary, &WaylandPrimaryInputMethod::insert, Qt::UniqueConnection);
    } else {
        qWarning() << "Cannot bind due to the protocol version mismatch. It requires the version 2 or higher";
    }
}

void WaylandPrimaryInputMethod::handleDestroy()
{
    qInfo() << "WaylandPrimaryInputMethod::handleDestroy" << this << displayId() << m_methods;

    // Secondaries will automatically clear m_methods via QPointer.
    // But the Primary is never destroyed, so manually clear the reference of m_methods.
    if (displayId() >= 0 && displayId() < m_methods.size())
        m_methods[displayId()] = nullptr;

    unbinding();
}

WaylandInputMethod *WaylandPrimaryInputMethod::getInputMethod(int displayId)
{
    if (displayId < 0 || displayId >= m_methods.size()) {
        qWarning() << "Cannot get inputMethod for displayId" << displayId;
        return nullptr;
    }

    return m_methods[displayId];
}

QList<QObject *> WaylandPrimaryInputMethod::methods()
{
    QList<QObject *> list;

    // Convert to the primitive type, <QObject *>, which can be directly interpreted in qml.
    for (int i = 0; i < m_methods.size(); ++i) {
        WaylandInputMethod *method = m_methods[i];
        list.append(method);
    }

    return list;
}

void WaylandPrimaryInputMethod::panelAdded(WaylandInputPanel *panel)
{
    for (int i = 0; i < m_methods.size(); ++i) {
        WaylandInputMethod *method = m_methods[i];
        if (method && method->client() == panel->client()) {
            method->setInputPanel(panel);
            return;
        }
    }

    qWarning() << "Cannot find inputMethod for panel" << panel;
}

WaylandTextModelFactoryDelegate* WaylandPrimaryInputMethod::waylandTextModelFactoryDelegate()
{
    return new WaylandTextModelFactoryDelegate();
}
