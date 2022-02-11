// Copyright (c) 2018-2022 LG Electronics, Inc.
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

#include "videooutputd_communicator.h"

VideoOutputdCommunicator* VideoOutputdCommunicator::m_instance = nullptr;

VideoOutputdCommunicator::VideoOutputdCommunicator(QObject *parent)
    : QObject(parent)
{
}

VideoOutputdCommunicator* VideoOutputdCommunicator::instance()
{
    if (m_instance == nullptr)
        m_instance = new VideoOutputdCommunicator(QCoreApplication::instance());

    return m_instance;
}

void VideoOutputdCommunicator::resetInstance()
{
    if (m_instance)
        delete m_instance;

    m_instance = nullptr;
}

void VideoOutputdCommunicator::setDisplayWindow(QRect sourceRectangle, QRect destinationRectangle, QString contextId)
{
    emit setVideoDisplayWindowRequested(sourceRectangle, destinationRectangle, contextId);
}

void VideoOutputdCommunicator::setCropRegion(QRect originalRectangle, QRect sourceRectangle, QRect destinationRectangle, QString contextId)
{
    emit setVideoCropRegionRequested(originalRectangle, sourceRectangle, destinationRectangle, contextId);
}

void VideoOutputdCommunicator::setProperty(QString name, QString value, QString contextId)
{
    emit setVideoPropertyRequested(name, value, contextId);
}
