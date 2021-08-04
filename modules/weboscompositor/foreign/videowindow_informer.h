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

#ifndef VIDEOWINDOW_INFORMER_H
#define VIDEOWINDOW_INFORMER_H

#include <QObject>
#include <QRect>
#include <QString>
#include <WebOSCoreCompositor/weboscompositorexport.h>

class WEBOS_COMPOSITOR_EXPORT VideoWindowInformer : public QObject
{
    Q_OBJECT
public:
    static VideoWindowInformer* instance();
    // Testing purpose only
    static void resetInstance();

    void insertVideoWindowList(const QString contextId, const QRect destinationRectangle, const QString windowId, const QString appId, const QRect appWindow);
    void removeVideoWindowList(const QString contextId);

signals:
    void insertVideoWindowInfo(const QString contextId, const QRect destinationRectangle, const QString windowId, const QString appId, const QRect appWindow);
    void removeVideoWindowInfo(const QString contextId);

protected:
    VideoWindowInformer(QObject *parent = Q_NULLPTR);

private:
    static VideoWindowInformer* m_instance;
};

#endif  //#ifndef VIDEOWINDOW_INFORMER_H
