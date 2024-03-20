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

#include "weboscompositorwindow.h"
#include "weboscorecompositor.h"
#include "webossurfaceitem.h"

#include "profiler.h"

Profiler::Profiler()
{
    m_lsmReady.start();
    m_eventLoopReady.start();
    m_firstAppMapped.start();
}

void Profiler::init(WebOSCoreCompositor *compositor, WebOSCompositorWindow *window)
{
    QObject::connect(compositor, &WebOSCoreCompositor::lsmReady, this, &Profiler::handleLSMReady);
    QObject::connect(compositor, &WebOSCoreCompositor::eventLoopReady, this, &Profiler::handleEventloopReady,
        Qt::QueuedConnection);
    QObject::connect(compositor, &WebOSCoreCompositor::surfaceMapped, this, &Profiler::handleFirstAppMapped);
}

void Profiler::handleLSMReady()
{
    qInfo() << "lsm-ready takes" << m_lsmReady.elapsed() << "ms";
}

void Profiler::handleEventloopReady()
{
    qInfo() << "event-loop-ready takes" << m_eventLoopReady.elapsed() << "ms";
}

void Profiler::handleFirstAppMapped(WebOSSurfaceItem *item)
{
    WebOSCoreCompositor *compositor = qobject_cast<WebOSCoreCompositor *>(sender());

    qInfo() << "first-app-mapped takes" << m_firstAppMapped.elapsed() << "ms appId =" << item->appId() << "pid =" << item->processId();

    QObject::disconnect(compositor, &WebOSCoreCompositor::surfaceMapped, this, &Profiler::handleFirstAppMapped);
}
