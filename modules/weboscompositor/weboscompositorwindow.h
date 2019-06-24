// Copyright (c) 2014-2019 LG Electronics, Inc.
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

#ifndef WEBOSCOMPOSITORWINDOW_H
#define WEBOSCOMPOSITORWINDOW_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QQmlEngine>
#include <QQuickView>
#include <QRunnable>
#include <QTimer>
#include <QUrl>

class WebOSCoreCompositor;
#ifdef USE_CONFIG
class WebOSCompositorConfig;
#endif

class WEBOS_COMPOSITOR_EXPORT WebOSCompositorWindow : public QQuickView {

    Q_OBJECT

    Q_PROPERTY(int displayId READ displayId CONSTANT)
    Q_PROPERTY(QRect outputGeometry READ outputGeometry NOTIFY outputGeometryChanged)
    Q_PROPERTY(int outputRotation READ outputRotation NOTIFY outputRotationChanged)
    Q_PROPERTY(bool outputClip READ outputClip NOTIFY outputClipChanged)
    Q_PROPERTY(bool outputGeometryPending READ outputGeometryPending WRITE setOutputGeometryPending NOTIFY outputGeometryPendingChanged)
    Q_PROPERTY(int outputGeometryPendingInterval READ outputGeometryPendingInterval WRITE setOutputGeometryPendingInterval NOTIFY outputGeometryPendingIntervalChanged)
    Q_PROPERTY(bool cursorVisible READ cursorVisible NOTIFY cursorVisibleChanged)

public:
    WebOSCompositorWindow(QString screenName = QString(), QString geometryString = QString(), QSurfaceFormat *surfaceFormat = 0);
    ~WebOSCompositorWindow();

    static QList<WebOSCompositorWindow *> initializeExtraWindows(int count);
    static bool parseGeometryString(QString string, QRect &geometry, int &rotation, double &ratio);

    void setCompositor(WebOSCoreCompositor* compositor);
    bool setCompositorMain(const QUrl& main);

    Q_INVOKABLE void showWindow();

    int displayId() const { return m_displayId; } ;

    QRect outputGeometry() const;
    int outputRotation() const;
    bool outputClip() const;

    void setBaseGeometry(const QRect& baseGeometry, const int rotation, const double outputRatio);
    Q_INVOKABLE void updateOutputGeometry(const int rotation, bool forced = false);

    bool outputGeometryPending() const;
    void setOutputGeometryPending(bool);
    int outputGeometryPendingInterval() const;
    void setOutputGeometryPendingInterval(int);

    void setDefaultCursor();
    void invalidateCursor();

    bool cursorVisible() const { return m_cursorVisible; }
    void setCursorVisible(bool visibility);
    Q_INVOKABLE void updateCursorFocus(Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    Q_INVOKABLE void updateForegroundItems(QList<QObject *>);
    QList<QObject *> foregroundItems() const { return m_foregroundItems; }

signals:
    void outputGeometryChanged();
    void outputRotationChanged();
    void outputClipChanged();
    void outputGeometryPendingChanged();
    void outputGeometryPendingIntervalChanged();

    void cursorVisibleChanged();

private:
    // classes
    class RotationJob : public QRunnable
    {
    public:
        RotationJob(WebOSCompositorWindow* window) { m_window = window; }
        void run() Q_DECL_OVERRIDE { m_window->sendOutputGeometry(); }
    private:
        WebOSCompositorWindow* m_window;
    };

    friend RotationJob;

    // methods
    void setNewOutputGeometry(QRect& outputGeometry, int outputRotation);
    void sendOutputGeometry() const;
    void applyOutputGeometry();

private slots:
    void onOutputGeometryDone();
    void onOutputGeometryPendingExpired();
    void onQmlError(const QList<QQmlError> &errors);

private:
    // variables
    WebOSCoreCompositor* m_compositor;
#ifdef USE_CONFIG
    WebOSCompositorConfig* m_config;
#endif

    int m_displayId;

    QUrl m_main;

    QRect m_baseGeometry;
    QRect m_outputGeometry;
    int m_outputRotation;
    bool m_outputClip;
    double m_outputRatio;

    QRect m_newOutputGeometry;
    int m_newOutputRotation;
    int m_newOutputRotationPending;

    bool m_outputGeometryPending;
    QTimer m_outputGeometryPendingTimer;
    int m_outputGeometryPendingInterval;

    bool m_cursorVisible;

    QList<QObject *> m_foregroundItems;
};
#endif // WEBOSCOMPOSITORWINDOW_H
