// Copyright (c) 2014-2020 LG Electronics, Inc.
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
#include <QQuickItem>
#include <QRunnable>
#include <QTimer>
#include <QUrl>
#include <QQuickItem>
#include <QPointer>

class QWaylandOutput;
class QWaylandSeat;
class WebOSCoreCompositor;
class WebOSSurfaceItem;
class WebOSCompositorPluginLoader;

class WEBOS_COMPOSITOR_EXPORT WebOSCompositorWindow : public QQuickView {

    Q_OBJECT

    Q_PROPERTY(int displayId READ displayId CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(bool accessible READ accessible WRITE setAccessible NOTIFY accessibleChanged)
    Q_PROPERTY(QRect outputGeometry READ outputGeometry NOTIFY outputGeometryChanged)
    Q_PROPERTY(int outputRotation READ outputRotation NOTIFY outputRotationChanged)
    Q_PROPERTY(bool outputClip READ outputClip NOTIFY outputClipChanged)
    Q_PROPERTY(bool outputGeometryPending READ outputGeometryPending WRITE setOutputGeometryPending NOTIFY outputGeometryPendingChanged)
    Q_PROPERTY(int outputGeometryPendingInterval READ outputGeometryPendingInterval WRITE setOutputGeometryPendingInterval NOTIFY outputGeometryPendingIntervalChanged)
    Q_PROPERTY(bool cursorVisible READ cursorVisible NOTIFY cursorVisibleChanged)
    // Fullscreen item of each window
    Q_PROPERTY(WebOSSurfaceItem *fullscreenItem READ fullscreenItem WRITE setFullscreenItem NOTIFY fullscreenItemChanged)
    Q_PROPERTY(QQuickItem *viewsRoot READ viewsRoot WRITE setViewsRoot NOTIFY viewsRootChanged)
    Q_PROPERTY(QVector<bool> isMirroringTo READ isMirroringTo NOTIFY isMirroringToChanged)
    Q_PROPERTY(MirroringState mirroringState READ mirroringState NOTIFY mirroringStateChanged)

public:
    enum MirroringState {
        MirroringStateInactive = 1,
        MirroringStateSender,
        MirroringStateReceiver,
        MirroringStateDisabled,
    };
    Q_ENUM(MirroringState);

    WebOSCompositorWindow(QString screenName = QString(), QString geometryString = QString(), QSurfaceFormat *surfaceFormat = 0);
    virtual ~WebOSCompositorWindow();

    static QList<WebOSCompositorWindow *> initializeExtraWindows(WebOSCoreCompositor* compositor, const int count, WebOSCompositorPluginLoader *pluginLoader = nullptr);
    static bool parseGeometryString(const QString string, QRect &geometry, int &rotation, double &ratio);

    void setCompositor(WebOSCoreCompositor* compositor);
    bool setCompositorMain(const QUrl& main, const QString& importPath = QString());

    Q_INVOKABLE void showWindow();

    int displayId() const { return m_displayId; }
    QString displayName() const { return m_displayName; }

    bool accessible() const { return m_accessible; }
    void setAccessible(bool enable);

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
    virtual void invalidateCursor();

    bool cursorVisible() const { return m_cursorVisible; }
    void setCursorVisible(bool visibility);
    Q_INVOKABLE void updateCursorFocus(Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    QQuickItem* viewsRoot() const { return m_viewsRoot; }
    void setViewsRoot(QQuickItem *viewsRoot);

    Q_INVOKABLE void updateForegroundItems(QList<QObject *>);
    QList<QObject *> foregroundItems() const { return m_foregroundItems; }

    void setOutput(QWaylandOutput *output) { m_output = output; }
    QWaylandOutput *output() { return m_output; }
    void setInputDevice(QWaylandSeat *device) { m_inputDevice = device; }
    QWaylandSeat *inputDevice() { return m_inputDevice; }
    WebOSSurfaceItem *fullscreenItem();
    void setFullscreenItem(WebOSSurfaceItem *item);

    Q_INVOKABLE int startMirroring(int target);
    Q_INVOKABLE int stopMirroring(int target);
    Q_INVOKABLE int stopMirroringToAll(WebOSSurfaceItem *source = nullptr);
    Q_INVOKABLE int stopMirroringFromTarget();
    int stopMirroringInternal(WebOSSurfaceItem *sItem, int targetId);
    QVector<bool> isMirroringTo();
    bool hasMirrorSource() const;
    WebOSCompositorWindow *mirrorSource() const { return m_mirrorSource; }
    void setMirrorSource(WebOSCompositorWindow *window);
    MirroringState mirroringState() const { return m_mirrorState; }
    void setMirroringState(MirroringState state);

    QString modelString();

signals:
    void outputGeometryChanged();
    void outputRotationChanged();
    void outputClipChanged();
    void outputGeometryPendingChanged();
    void outputGeometryPendingIntervalChanged();

    void cursorVisibleChanged();
    void viewsRootChanged();
    void fullscreenItemChanged(WebOSSurfaceItem *);
    void isMirroringToChanged();
    void mirroringStateChanged();

    void accessibleChanged(const bool enabled);

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
    void onFullscreenItemChanged(WebOSSurfaceItem *oldItem);
    void onQmlError(const QList<QQmlError> &errors);

private:
    // variables
    WebOSCoreCompositor* m_compositor;

    int m_displayId;
    QString m_displayName;

    bool m_accessible = false;

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

    QQuickItem* m_viewsRoot;
    QList<QObject *> m_foregroundItems;
    QWaylandOutput *m_output;
    QWaylandSeat *m_inputDevice;
    // auto clear on destroyed
    QPointer<WebOSSurfaceItem> m_fullscreenItem;
    WebOSCompositorWindow *m_mirrorSource;
    MirroringState m_mirrorState = MirroringStateInactive;
};
#endif // WEBOSCOMPOSITORWINDOW_H
