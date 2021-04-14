// Copyright (c) 2021 LG Electronics, Inc.
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

#include "webossurface.h"
#include <QtWaylandCompositor/private/qwaylandquicksurface_p.h>

class WebOSSurfacePrivate : public QWaylandQuickSurfacePrivate
{
    Q_DECLARE_PUBLIC(WebOSSurface)
public:
    WebOSSurfacePrivate()
    {
    }

    ~WebOSSurfacePrivate() override
    {
    }

protected:
    void surface_destroy_resource(Resource *resource) override;
    void surface_attach(Resource *resource, struct ::wl_resource *buffer, int32_t x, int32_t y) override;
};

void WebOSSurfacePrivate::surface_destroy_resource(Resource *resource)
{
    Q_Q(WebOSSurface);
    emit q->aboutToBeDestroyed();
    QWaylandQuickSurfacePrivate::surface_destroy_resource(resource);
}

void WebOSSurfacePrivate::surface_attach(Resource *resource, struct ::wl_resource *buffer, int32_t x, int32_t y)
{
    Q_Q(WebOSSurface);

    QWaylandQuickSurfacePrivate::surface_attach(resource, buffer, x, y);

    if (!pending.buffer.hasContent())
        emit q->nullBufferAttached();
}

WebOSSurface::WebOSSurface()
    : QWaylandQuickSurface(*new WebOSSurfacePrivate())
{
}
