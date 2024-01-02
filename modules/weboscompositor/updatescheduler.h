// Copyright (c) 2021-2022 LG Electronics, Inc.
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

#ifndef UPDATESCHEDULER_H
#define UPDATESCHEDULER_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QQueue>
#include <QTimer>
#include <QElapsedTimer>
#include <QLoggingCategory>

class WebOSCompositorWindow;

class WEBOS_COMPOSITOR_EXPORT UpdateScheduler : public QObject{
    Q_OBJECT
public:
    UpdateScheduler(WebOSCompositorWindow *);

    void init();
    bool isAdaptiveUpdate() { return m_adaptiveUpdate; }

    bool updateRequested();

    void checkTimeout();

    void surfaceDamaged();
    void sendFrame();

    void frameStarted();

    bool vsyncCorrection();

    static void pageFlipNotifier(WebOSCompositorWindow* win, unsigned int seq, unsigned int tv_sec, unsigned int tv_usec);

signals:
    void frameMissed();
    void pageFlipped(quint32, quint32, quint32);
    void legacyPageFlipped();

private slots:
    void frameFinished();
    void profileFrame(quint32, quint32, quint32);
    void onFrameSwapped();
    void onFrameMissed();

    void deliverUpdateRequest();

    void scheduleFrameCallback();

    void setNextUpdateWithDefaultNotifier();
    void onBeforeSynchronizing();
    void onBeforeRendering();
    void onAfterRendering();
    void onLegacyPageFlipped();
    void frameTimeout();

private:
    WebOSCompositorWindow *m_window = nullptr;

    qreal m_vsyncInterval = 1.0 / 60 * 1000;

    bool m_adaptiveUpdate = false;

    QTimer m_updateTimer;
    bool m_hasUnhandledUpdateRequest = false;
    int m_updateTimerInterval = 0;

    bool m_adaptiveFrame = false;
    QTimer m_frameTimer;
    int m_frameTimerInterval = 0;

    quint32 m_frameCount = 0;
    quint32 m_framesOnUpdate = 0;

    int m_frameToDamaged = 0;

    int m_vsyncNsecsInterval = 1000000000 / 60;

    QTimer m_frameWatcherTimer;

    //Debug Timers
    QElapsedTimer m_sinceSendFrame;
    QElapsedTimer m_sinceSurfaceDamaged;
    QElapsedTimer m_vsyncElapsedTimer;
    QQueue<QElapsedTimer> m_frameTimerQueue;

    // Legacy adaptive update
    QElapsedTimer m_sinceSyncStart;
    int m_timeSpentForRendering;
};

#endif // UPDATESCHEDULER_H
