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

#ifndef WEBOSSURFACE_H
#define WEBOSSURFACE_H

#include <QtCore/qglobal.h>
#include <QObject>
#include <QtWaylandCompositor/qwaylandquickchildren.h>
#include <QtWaylandCompositor/qwaylandquicksurface.h>

class WebOSSurfacePrivate;

class WebOSSurface : public QWaylandQuickSurface
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WebOSSurface)
    Q_WAYLAND_COMPOSITOR_DECLARE_QUICK_CHILDREN(WebOSSurface)

public:
    WebOSSurface();

signals:
    void aboutToBeDestroyed();
    void nullBufferAttached();
};

#endif // WEBOSSURFACE_H
