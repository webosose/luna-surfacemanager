// Copyright (c) 2024 LG Electronics, Inc.
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

#ifndef PROFILER_H
#define PROFILER_H

#include <QObject>
#include <QElapsedTimer>

#include "weboscompositorexport.h"

class WebOSCoreCompositor;
class WebOSCompositorWindow;
class WebOSSurfaceItem;

class WEBOS_COMPOSITOR_EXPORT Profiler : public QObject
{
    Q_OBJECT
public:
    Profiler();
    void init(WebOSCoreCompositor *compositor, WebOSCompositorWindow *window);

public slots:
    void handleLSMReady();
    void handleEventloopReady();
    void handleFirstAppMapped(WebOSSurfaceItem *item);

private:
    QElapsedTimer m_lsmReady;
    QElapsedTimer m_eventLoopReady;
    QElapsedTimer m_firstAppMapped;
};

#endif
