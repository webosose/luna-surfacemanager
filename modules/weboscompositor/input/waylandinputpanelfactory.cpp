// Copyright (c) 2019-2020 LG Electronics, Inc.
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

#include "waylandprimaryinputmethod.h"
#include "waylandinputpanelfactory.h"
#include "waylandinputpanel.h"

WaylandInputPanelFactory::WaylandInputPanelFactory(QWaylandCompositor* compositor, WaylandPrimaryInputMethod *inputMethod)
    : m_compositor(compositor)
    , m_inputMethod(inputMethod)
{
    wl_display_add_global(compositor->display(), &input_panel_interface, this, WaylandInputPanelFactory::bind);
}

WaylandInputPanelFactory::~WaylandInputPanelFactory()
{
}

void WaylandInputPanelFactory::bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    Q_UNUSED(version);
    WaylandInputPanelFactory* that = static_cast<WaylandInputPanelFactory*>(data);
    WaylandInputPanel *panel = new WaylandInputPanel(client, id);

    qInfo() << panel << "bound";
    that->panelAdded(panel);
}

void WaylandInputPanelFactory::panelAdded(WaylandInputPanel *panel)
{
    m_inputMethod->panelAdded(panel);
}
