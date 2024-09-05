// Copyright (c) 2021-2024 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use m_window file except in compliance with the License.
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

#include <qpa/qplatformscreen.h>
#include <qpa/qplatformnativeinterface.h>

#include "updatescheduler.h"
#include "weboscompositorwindow.h"
#include "weboscompositortracer.h"
#include "securecoding.h"

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include <QGuiApplication>
#include <QScreen>
#include <QtMath>
#endif

static constexpr int STATIC_SWAP_BUFFER_TIME = 2;

/* Increasing this value, frame_callback is sent earlier,
   so that client starts to update earlier. Eventually,
   frame latency will be increased as the amount of the time.
   If the value is too small, client may miss next vsync. */
static constexpr int RENDER_FLUCTUATION_BUFFER_TIME = 4;

/* Initial and maximum value to be idle before update.
   In rpi4, rough exeprimental times are
     - UpdateRequest to SwapBuffer : ~3ms
     - SwapBuffer to on-screen Presentation: ~5ms
   So the default idle time is 16.6 - 8ms ~= 8ms
   It can be tunnable for each backend. */
static int s_default_update_idle_time = qgetenv("WEBOS_UPDATE_IDLE_TIME").toInt();
static bool debug_render = qgetenv("WEBOS_UPDATE_DEBUG").toInt() == 1;

/* This is from another thread which handles drm event */
void UpdateScheduler::pageFlipNotifier(WebOSCompositorWindow* win, unsigned int seq, unsigned int tv_sec, unsigned int tv_usec)
{
    PMTRACE_FUNCTION;

    static unsigned int last_sequence = 0;
    static unsigned int last_sec = 0;
    static unsigned int last_usec = 0;
    UpdateScheduler *upsched = win ? win->updateScheduler() : nullptr;

    if (debug_render) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        qDebug() << "timestamp" << ts.tv_sec << ts.tv_nsec / 1000 << "us";
        qDebug() << upsched << "seq" << seq << "Backend timestamp" << tv_sec << tv_usec << "us";
    }

    // NOTE: To be optimized for each backend
    if (upsched && seq != 0) {
        if (seq != last_sequence + 1)
            emit upsched->frameMissed();

        emit upsched->pageFlipped(seq, tv_sec, tv_usec*1000);
    }

    last_sequence = seq;
    last_sec = tv_sec;
    last_usec = tv_usec;
}

UpdateScheduler::UpdateScheduler(WebOSCompositorWindow *window)
    : m_window(window)
{
    init();
}

void UpdateScheduler::init()
{
    static int defaultUpdateInterval = []() {
        bool ok = false;
        int customUpdateInterval = qEnvironmentVariableIntValue("QT_QPA_UPDATE_IDLE_TIME", &ok);
        return ok ? customUpdateInterval : 5;
    }();

    if (!m_window)
        return;

    bool hasPageFlipNotifier = m_window->hasPageFlipNotifier();

    // Get VSync interval
    m_vsyncInterval = 1.0 / m_window->screen()->refreshRate() * 1000;

    m_adaptiveUpdate = (qgetenv("WEBOS_COMPOSITOR_ADAPTIVE_UPDATE").toInt() == 1);
    m_adaptiveFrame = (qgetenv("WEBOS_COMPOSITOR_ADAPTIVE_FRAME_CALLBACK").toInt() == 1);

    // Adaptive frame callback requires adaptive update
    if (m_adaptiveFrame && !m_adaptiveUpdate)
        m_adaptiveFrame = false;

    if (!m_adaptiveUpdate) {
        qInfo() << "Default update interval" << defaultUpdateInterval << "for window" << m_window << "vsyncInterval:" << m_vsyncInterval;
        return;
    }

    qInfo() << "Adaptive update interval for window" << m_window << "vsyncInterval:" << m_vsyncInterval;

    if (defaultUpdateInterval != 0)
        qWarning() << "QT_QPA_UPDATE_IDLE_TIME should be 0 but" << defaultUpdateInterval;

    m_updateTimerInterval = s_default_update_idle_time; // changes adaptively per every frame
    m_updateTimer.setTimerType(Qt::PreciseTimer);
    m_updateTimer.setSingleShot(true);
    connect(&m_updateTimer, &QTimer::timeout, this, &UpdateScheduler::deliverUpdateRequest);

    // Adaptive Frame
    m_window->output()->setAutomaticFrameCallback(!m_adaptiveFrame);

    if (m_adaptiveFrame) {
        m_frameTimerInterval = double2int(m_vsyncInterval / 3); // changes adaptively per every frame
        m_frameTimer.setTimerType(Qt::PreciseTimer);
        m_frameTimer.setSingleShot(true);

        connect(&m_frameTimer, &QTimer::timeout, this, &UpdateScheduler::sendFrame);
        connect(m_window, &QQuickWindow::afterRendering, this, &UpdateScheduler::scheduleFrameCallback);
        qInfo() << "Adaptive frame callback for window" << m_window << m_frameTimerInterval;
    }

    qInfo() << "Adaptive update with pageflip notifier" << hasPageFlipNotifier;

    // For legacy adaptive update
    if (!hasPageFlipNotifier) {
        connect(m_window, &QQuickWindow::beforeSynchronizing, this, &UpdateScheduler::onBeforeSynchronizing);
        connect(m_window, &QQuickWindow::afterRendering, this, &UpdateScheduler::onAfterRendering);
        connect(m_window, &QQuickWindow::frameSwapped, this, &UpdateScheduler::setNextUpdateWithDefaultNotifier);
        connect(this, &UpdateScheduler::legacyPageFlipped, this, &UpdateScheduler::onLegacyPageFlipped);
    } else {
        // scheduling next update in frameFinished
        connect(this, &UpdateScheduler::pageFlipped, this, &UpdateScheduler::frameFinished);
        connect(this, &UpdateScheduler::frameMissed, this, &UpdateScheduler::onFrameMissed);
        connect(m_window, &QQuickWindow::frameSwapped, this, &UpdateScheduler::onFrameSwapped);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        connect(m_window, &QQuickWindow::renderingAborted, this, &UpdateScheduler::renderingAborted);
#endif

        // Debugging purpose
        if (debug_render)
            connect(this, &UpdateScheduler::pageFlipped, this, &UpdateScheduler::profileFrame);
    }
}

bool UpdateScheduler::updateRequested()
{
    PMTRACE_FUNCTION;

    if (!m_adaptiveUpdate)
        return false;

    if (m_updateTimer.isActive() || m_frameSwapped == false || m_framesOnUpdate != 0) {
        if (debug_render) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            qDebug() << "sinceDamaged:" << m_sinceSurfaceDamaged.elapsed() << "ms" << "framesOnUpdate" << m_framesOnUpdate << "timestamp" << ts.tv_sec << ts.tv_nsec / 1000 << "us";
            qDebug() << "updateTimer:" << m_updateTimer.isActive() << "m_frameSwapped:" << m_frameSwapped;
        }

        m_hasUnhandledUpdateRequest = true;
        return true;
    }
    // First UpdateRequest, just fall through to start sync

    if (debug_render)
        qDebug() << "updateRequested without timer." << "sinceDamaged:" << m_sinceSurfaceDamaged.elapsed() << "ms";
    // This path will immediately handle UpdateRequest, so start to track the frame
    frameStarted();
    return false;
}

void UpdateScheduler::deliverUpdateRequest()
{
    PMTRACE_FUNCTION;

    if (m_adaptiveUpdate) {
        // Send any unhandled UpdateRequest
        if (m_hasUnhandledUpdateRequest) {
            if (m_frameSwapped == false) {
                if (debug_render) {
                    qDebug() << "Skip update since onFrameSwapped is not called after frameFinished";
                }
                return;
            }

            m_hasUnhandledUpdateRequest = false;

            // Start to keep track the frame
            frameStarted();
            m_window->deliverUpdateRequest();
        } else if (debug_render) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);

            qDebug() << "no update since framecallback. timestamp" << ts.tv_sec << ts.tv_nsec / 1000 << "us";
        }
    } else {
        qWarning() << "deliverUpdateRequest is called for" << m_window << "while adaptiveUpdate is NOT set!";
    }
}

// It is proper to be called in event() to handle delayed UpdateRequest in first
void UpdateScheduler::checkTimeout()
{
    PMTRACE_FUNCTION;

    // When the remaining time of QTimer is 0, it means that the timer has
    // expired but is being delayed due to other events and we need to
    // take actions needed right away.
    if (m_adaptiveUpdate && m_updateTimer.remainingTime() == 0) {
        if (debug_render)
            qDebug() << "Timeout for updateTimer";
        m_updateTimer.stop();
        deliverUpdateRequest();
    }

    if (m_adaptiveFrame && m_frameTimer.remainingTime() == 0) {
        if (debug_render)
            qDebug() << "Timeout for frameTimer";
        m_frameTimer.stop();
        sendFrame();
    }
}

void UpdateScheduler::surfaceDamaged()
{
    PMTRACE_FUNCTION;

    static QElapsedTimer damagedInterval;

    if (m_adaptiveFrame && m_sinceSendFrame.isValid()) {
        m_frameToDamaged = m_sinceSendFrame.elapsed();

        m_sinceSendFrame.invalidate();
        m_sinceSurfaceDamaged.start();

        if (debug_render)
            qDebug() << "frameToDamaged" << m_frameToDamaged << "ms";
    }
    damagedInterval.start();
}

void UpdateScheduler::sendFrame()
{
    static QElapsedTimer frameInterval;
    PMTRACE_FUNCTION;
    QWaylandOutput *output = m_window->output();
    if (m_adaptiveFrame && output) {
        if (m_sinceSendFrame.isValid()) {
            // No surfaceDamaged called since the last frame.
            // It means that rendering time on client exceeds the vsync interval.
            m_frameTimerInterval = 0;
        }
        m_sinceSendFrame.start();
        output->sendFrameCallbacks();

        if (m_sinceSurfaceDamaged.isValid()) {
            qint64 damageToFrame = m_sinceSurfaceDamaged.elapsed();
            if (debug_render)
                qDebug() << "damagedToFrame" <<  damageToFrame << "ms" << "vsyncElapsed" << m_vsyncElapsedTimer.elapsed() << "ms" << "frame interval" << frameInterval.elapsed() << "timer" << m_frameTimerInterval;
            if (damageToFrame > m_vsyncInterval * 2 && debug_render)
                qDebug() << "Elapsed since surface damaged until sending a frame callback:" << damageToFrame << "ms";

            m_sinceSurfaceDamaged.invalidate();
        }
        frameInterval.start();
    }
}

void UpdateScheduler::scheduleFrameCallback()
{
    PMTRACE_FUNCTION;

    if (!m_vsyncElapsedTimer.isValid())
        return;
    // How long takes from last vsync
    int elapsed = (m_vsyncElapsedTimer.nsecsElapsed() % m_vsyncNsecsInterval) / 1000000;
    int remain = double2int(m_vsyncInterval - elapsed);
    int nextFrameTime = remain + m_updateTimerInterval - (m_frameToDamaged + RENDER_FLUCTUATION_BUFFER_TIME);
    nextFrameTime = nextFrameTime < 0 ? 0 : nextFrameTime;
    m_frameTimerInterval = nextFrameTime <= m_frameTimerInterval
        ? nextFrameTime : m_frameTimerInterval + 1;

    if (debug_render)
        qDebug() << "sinceVsync" << elapsed << "m_frameToDamaged" << m_frameToDamaged << "next" << nextFrameTime << "timer" << m_frameTimerInterval;

    if (m_adaptiveFrame) {
        if (m_frameTimer.isActive()) {
            qDebug() << "frameTimer is still active, skipped";
            return;
        }
        m_frameTimer.start(m_frameTimerInterval);
    }
}

// Legacy adaptive update
inline __attribute__((always_inline)) static int calculateUpdateTimerInterval(
    int timeSinceRenderingToSwapBuffer, qreal vsyncInterval, int updateTimerInterval)
{
    int nextInterval = double2int(vsyncInterval - (timeSinceRenderingToSwapBuffer + RENDER_FLUCTUATION_BUFFER_TIME));
    nextInterval = nextInterval > 0 ? nextInterval : updateTimerInterval;
    updateTimerInterval = updateTimerInterval >= nextInterval ?
        nextInterval : updateTimerInterval + 1;
    return updateTimerInterval;
}

void UpdateScheduler::onBeforeSynchronizing()
{
    PMTRACE_FUNCTION;
    m_sinceSyncStart.start();
}

void UpdateScheduler::renderingAborted() {
    PMTRACE_FUNCTION;

    qInfo() << "renderingAborted" << "m_hasUnhandledUpdateRequest:" << m_hasUnhandledUpdateRequest;

    m_framesOnUpdate = 0;
    m_frameSwapped = true;

    if(debug_render) {
        m_frameTimerQueue.clear();
    }

    if(m_hasUnhandledUpdateRequest){
        deliverUpdateRequest();
    }
}

void UpdateScheduler::onAfterRendering()
{
    PMTRACE_FUNCTION;
    m_timeSpentForRendering = m_sinceSyncStart.elapsed();
}

void UpdateScheduler::setNextUpdateWithDefaultNotifier()
{
    PMTRACE_FUNCTION;
    // Assume that page flip happens just after the swapbuffer with the constant time
    int timeSinceRenderingToSwapBuffer = m_timeSpentForRendering + STATIC_SWAP_BUFFER_TIME;
    m_updateTimerInterval =
        calculateUpdateTimerInterval(timeSinceRenderingToSwapBuffer,
                                     m_vsyncInterval, m_updateTimerInterval);
    emit legacyPageFlipped();
}

void UpdateScheduler::onLegacyPageFlipped()
{
    if (Q_LIKELY(m_framesOnUpdate > 0))
        m_framesOnUpdate--;

    if (m_adaptiveUpdate) {
        m_updateTimer.start(m_updateTimerInterval);
    }

    if (m_adaptiveFrame)
        m_frameTimer.start(m_frameTimerInterval);

    m_frameSwapped = true;
}
// Legacy adaptive update

void UpdateScheduler::onFrameMissed()
{
    PMTRACE_FUNCTION;

    if (debug_render)
        qDebug() << "FrameMissed";
    m_updateTimerInterval--;

    if (m_updateTimerInterval < 0)
        m_updateTimerInterval = 0;

    m_frameCount = 0;
}

void UpdateScheduler::frameStarted()
{
    PMTRACE_FUNCTION;

    QElapsedTimer timer;
    timer.start();

    m_framesOnUpdate++;
    if (m_framesOnUpdate > 1) {
        qWarning() << "frames on update is " << m_framesOnUpdate << " (over 1)";
    }

    if (debug_render) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        qDebug() << "sinceDamaged:" << m_sinceSurfaceDamaged.elapsed() << "ms" << "sinceVsync" << m_vsyncElapsedTimer.elapsed() << "ms" << "timestamp" << ts.tv_sec << ts.tv_nsec / 1000 << "us";
        m_frameTimerQueue << timer;
    }

    m_frameSwapped = false;
}

void UpdateScheduler::onFrameSwapped()
{
    PMTRACE_FUNCTION;

    if (debug_render) {
        int updateRequestToSwap = 0;
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        qDebug() << "timestamp" << ts.tv_sec << ts.tv_nsec / 1000 << "us";

        if (Q_LIKELY(!m_frameTimerQueue.isEmpty())) {
            QElapsedTimer timer = m_frameTimerQueue.head();
            updateRequestToSwap = timer.nsecsElapsed()/1000;
            qDebug() << "updateRequestToSwap" << updateRequestToSwap << "us";
        }
    }

    if(m_framesOnUpdate == 0) {
        qDebug() << "onFrameSwapped is called after frameFinished";
        if(!m_updateTimer.isActive()) {
            m_updateTimer.start(0);
        }
    }

    m_frameSwapped = true;
}

/* If there is page_flip_notifier, then the finish of the frame is regarded as on-screen presentation. */
void UpdateScheduler::frameFinished()
{
    PMTRACE_FUNCTION;
    static int threshHold = qgetenv("WEBOS_UPDATE_FRAMEMISS_THRESHHOLD").toInt();

    if (Q_LIKELY(m_framesOnUpdate > 0))
        m_framesOnUpdate--;

    m_frameCount++;
    // Try to have more idle time if frame hits on time for some duration.
    if (m_frameCount >= threshHold) {
        m_updateTimerInterval++;
        m_frameCount = 0;
    }

    // Check initial maximum interval
    m_updateTimerInterval = qMin(m_updateTimerInterval, s_default_update_idle_time);

    if (m_adaptiveUpdate && m_framesOnUpdate == 0) {
        m_updateTimer.start(m_updateTimerInterval);
    }

    if (debug_render)
        qDebug() << "m_framesOnUpdate" << m_framesOnUpdate << "m_updateTimerInterval" << m_updateTimerInterval << "m_frameTimerInterval" << m_frameTimerInterval;

    m_vsyncElapsedTimer.start();
}

void UpdateScheduler::profileFrame(quint32 seq, quint32 tv_sec, quint32 tv_nsec)
{
    static QElapsedTimer frameIntervalTimer;
    int updateRequestToFlip = 0; // how long takes from update request to finish of frame.

    if (Q_LIKELY(!m_frameTimerQueue.isEmpty())) {
        QElapsedTimer timer = m_frameTimerQueue.dequeue();
        updateRequestToFlip = timer.nsecsElapsed()/1000;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    qDebug() << "updateRequestToFlip" << updateRequestToFlip << "us framesInterval" << frameIntervalTimer.nsecsElapsed()/1000000.0 << "timestamp" << ts.tv_sec << ts.tv_nsec/1000 << "us";
    emit m_window->frameProfileUpdated(updateRequestToFlip, frameIntervalTimer.nsecsElapsed()/1000);
    frameIntervalTimer.start();
}
