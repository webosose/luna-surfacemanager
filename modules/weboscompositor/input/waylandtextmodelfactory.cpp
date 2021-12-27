// Copyright (c) 2013-2022 LG Electronics, Inc.
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

#include <QWaylandCompositor>
#include <QDebug>

#include "waylandtextmodelfactory.h"
#include "waylandtextmodel.h"
#include "waylandprimaryinputmethod.h"

const struct text_model_factory_interface WaylandTextModelFactory::textModelFactoryInterface = {
    WaylandTextModelFactory::createTextModel
};

WaylandTextModelFactory::WaylandTextModelFactory(QWaylandCompositor* compositor, WaylandPrimaryInputMethod* inputMethod)
    : m_inputMethod(inputMethod)
    , m_delegate(inputMethod->waylandTextModelFactoryDelegate())
{
    wl_display_add_global(compositor->display(), &text_model_factory_interface, this, WaylandTextModelFactory::wlBindFactory);
}

void WaylandTextModelFactory::wlBindFactory(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    WaylandTextModelFactory* that = static_cast<WaylandTextModelFactory*>(data);
    wl_client_add_object(client, &text_model_factory_interface, &textModelFactoryInterface, id, that);
}

void WaylandTextModelFactory::createTextModel(struct wl_client *client,
                                            struct wl_resource *resource,
                                            uint32_t id)
{
    WaylandTextModelFactory* that = static_cast<WaylandTextModelFactory*>(resource->data);
    that->m_delegate->createTextModel(that, client, resource, id);
}

WaylandTextModelFactory::~WaylandTextModelFactory()
{
}

WaylandInputMethod *WaylandTextModelFactory::findInputMethod(int displayId)
{
    qInfo() << "Trying to findInputMethod" << displayId;
    return m_inputMethod->getInputMethod(displayId);
}

void WaylandTextModelFactoryDelegate::createTextModel(WaylandTextModelFactory *factory,
                                                      struct wl_client *client,
                                                      struct wl_resource *resource,
                                                      uint32_t id)
{
    // The life cycle management of these model object comes from wayland
    // see destroy methods from respective classes
    WaylandTextModel* model = new WaylandTextModel(factory, client, resource, id);
}
