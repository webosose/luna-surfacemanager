// Copyright (c) 2014-2022 LG Electronics, Inc.
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

#ifndef WEBOSSURFACEGROUPCOMPOSITOR_H
#define WEBOSSURFACEGROUPCOMPOSITOR_H

#include <WebOSCoreCompositor/private/qwayland-server-webos-surface-group.h>

#include <QObject>
#include <QMap>

class WebOSCoreCompositor;
class WebOSSurfaceItem;
class WebOSSurfaceGroup;

class WebOSSurfaceGroupCompositor : public QObject, public QtWaylandServer::wl_webos_surface_group_compositor {

    Q_OBJECT

public:
    WebOSSurfaceGroupCompositor(WebOSCoreCompositor* compositor);

    void removeGroup(WebOSSurfaceGroup* group);

    WebOSCoreCompositor* compositor() { return m_compositor; }

    WebOSSurfaceGroup* surfaceGroup(const QString &name) { return m_groups.value(name, nullptr); }

protected:
    void webos_surface_group_compositor_create_surface_group(Resource *resource, uint32_t id,
                                                             struct ::wl_resource *parent, const QString &name) override;
    void webos_surface_group_compositor_get_surface_group(Resource *resource, uint32_t id, const QString &name) override;


private:
    WebOSCoreCompositor* m_compositor;
    QMap<QString, WebOSSurfaceGroup*> m_groups;

};
#endif
