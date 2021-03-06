// Copyright (c) 2013-2020 LG Electronics, Inc.
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

#ifndef COMPOSITORWINDOWCLOSE_H
#define COMPOSITORWINDOWCLOSE_H

#include <QObject>
#include <QJsonObject>
#include <compositorextensionglobal.h>

QT_BEGIN_NAMESPACE

class QWaylandQuickItem;

class WEBOS_COMPOSITOR_EXT_EXPORT CompositorWindowClose : public QObject
{
    Q_OBJECT
public:
    virtual void close(QWaylandQuickItem *item, QJsonObject payload = QJsonObject()) { Q_UNUSED(item); Q_UNUSED(payload); }
};

QT_END_NAMESPACE

#endif // COMPOSITORWINDOWCLOSE_H
