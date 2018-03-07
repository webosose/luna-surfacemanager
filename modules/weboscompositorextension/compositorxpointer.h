// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#ifndef COMPOSITORXPOINTER_H
#define COMPOSITORXPOINTER_H

#include <QObject>
#include <compositorextensionglobal.h>

QT_BEGIN_NAMESPACE

class QWaylandSurface;

class WEBOS_COMPOSITOR_EXT_EXPORT CompositorXPointer: public QObject
{
    Q_OBJECT
public:
    virtual void pointerEntered(QWaylandSurface *surface) { Q_UNUSED(surface); }
    virtual void pointerLeaved(QWaylandSurface *surface) { Q_UNUSED(surface); }
};

QT_END_NAMESPACE

#endif // COMPOSITORXPOINTER_H
