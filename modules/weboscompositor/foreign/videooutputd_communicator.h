// Copyright (c) 2013-2022 LG Electronics, Inc.
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

#ifndef VIDEOOUTPUTD_COMMUNICATOR_H
#define VIDEOOUTPUTD_COMMUNICATOR_H

#include <QObject>
#include <QRect>
#include <QString>
#include <WebOSCoreCompositor/weboscompositorexport.h>

class WEBOS_COMPOSITOR_EXPORT VideoOutputdCommunicator : public QObject
{
    Q_OBJECT
public:
    static VideoOutputdCommunicator* instance();
    // Testing purpose only
    static void resetInstance();

    void setDisplayWindow(QRect sourceRectangle, QRect destinationRectangle, QString contextId);
    void setCropRegion(QRect originalRectangle, QRect sourceRectangle, QRect destinationRectangle, QString contextId);
    void setProperty(QString name, QString value, QString contextId);

signals:
    void setVideoDisplayWindowRequested(const QRect sourceRectangle, const QRect destinationRectangle, const QString contextId);
    void setVideoCropRegionRequested(const QRect originalRectangle, const QRect sourceRectangle, const QRect destinationRectangle, const QString contextId);
    void setVideoPropertyRequested(const QString name, const QString value, const QString contextId);

protected:
    VideoOutputdCommunicator(QObject *parent = Q_NULLPTR);

private:
    static VideoOutputdCommunicator* m_instance;
};

#endif  //#ifndef VIDEOOUTPUTD_COMMUNICATOR_H
