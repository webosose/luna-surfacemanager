// Copyright (c) 2019-2022 LG Electronics, Inc.
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

#include <QCoreApplication>
#include <QDebug>

#include "videowindow_informer.h"

VideoWindowInformer* VideoWindowInformer::m_instance = nullptr;

VideoWindowInformer::VideoWindowInformer(QObject *parent)
    : QObject(parent)
{
}

VideoWindowInformer* VideoWindowInformer::instance()
{
    if (m_instance == nullptr)
        m_instance = new VideoWindowInformer(QCoreApplication::instance());

    return m_instance;
}

void VideoWindowInformer::resetInstance()
{
    if (m_instance)
        delete m_instance;

    m_instance = nullptr;
}

void VideoWindowInformer::insertVideoWindowList(const QString contextId, const QRect destinationRectangle, const QString windowId, const QString appId, const QRect appWindow)
{
    qDebug() << "insertVideoWindowList() with contextId : " << contextId << " , destinationRectangle : " << destinationRectangle << " , windowId : " << windowId << "  ,  appId " << appId << " , appWindow : " << appWindow;
    emit insertVideoWindowInfo(contextId, destinationRectangle, windowId, appId, appWindow);
}

void VideoWindowInformer::removeVideoWindowList(const QString contextId)
{
    qDebug() << "removeVideoWindowList() with contextId : " << contextId;
    emit removeVideoWindowInfo(contextId);
}
