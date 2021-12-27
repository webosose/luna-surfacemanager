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

#ifndef WAYLANDTEXTMODELFACTORY_H
#define WAYLANDTEXTMODELFACTORY_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <wayland-server.h>
#include <wayland-text-server-protocol.h>

class WaylandInputMethod;
class WaylandPrimaryInputMethod;
class WaylandTextModelFactory;

class WEBOS_COMPOSITOR_EXPORT WaylandTextModelFactoryDelegate {
public:
    virtual ~WaylandTextModelFactoryDelegate() {}
    virtual void createTextModel(WaylandTextModelFactory *factory,
                                 struct wl_client *client,
                                 struct wl_resource *resource,
                                 uint32_t id);
};

class WaylandTextModelFactory {

public:
    WaylandTextModelFactory(QWaylandCompositor* compositor, WaylandPrimaryInputMethod* inputMethod);
    ~WaylandTextModelFactory();

    WaylandInputMethod *findInputMethod(int displayId);

private:
    static const struct text_model_factory_interface textModelFactoryInterface;
    static void wlBindFactory(struct wl_client *client, void *data, uint32_t version, uint32_t id);

    static void createTextModel(struct wl_client *client,
                                struct wl_resource *resource,
                                uint32_t id);
private:
    WaylandPrimaryInputMethod *m_inputMethod;
    QScopedPointer<WaylandTextModelFactoryDelegate> m_delegate;
};

#endif //WAYLANDTEXTMODELFACTORY_H
