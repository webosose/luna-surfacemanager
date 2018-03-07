// Copyright (c) 2013-2018 LG Electronics, Inc.
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

#include "compositorextension.h"
#include <QDebug>
QT_BEGIN_NAMESPACE

CompositorExtension::CompositorExtension()
    : m_compositor(0)
{
}

void CompositorExtension::setCompositor(QWaylandCompositor *compositor)
{
    m_compositor = compositor;

    init();
}

void CompositorExtension::handleItemExposed(QString &item)
{
    if (item == QString::fromLatin1("home"))
        handleHomeExposed();
    /* Handle other items if needed */
}

void CompositorExtension::handleItemUnexposed(QString &item)
{
    if (item == QString::fromLatin1("home"))
        handleHomeUnexposed();
    /* Handle other items if needed */
}

QT_END_NAMESPACE
