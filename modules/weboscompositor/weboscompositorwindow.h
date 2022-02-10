// Copyright (c) 2014-2021 LG Electronics, Inc.
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
#include <QElapsedTimer>

#include <QWaylandQuickOutput>

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
    Q_PROPERTY(QQuickItem *viewsRoot READ viewsRoot WRITE setViewsRoot NOTIFY viewsRootChanged)
    // State for App mirroring
    Q_PROPERTY(AppMirroringState appMirroringState READ appMirroringState NOTIFY appMirroringStateChanged)
    Q_PROPERTY(WebOSSurfaceItem *appMirroringItem READ appMirroringItem WRITE setAppMirroringItem NOTIFY appMirroringItemChanged)
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    Q_MOC_INCLUDE("webossurfaceitem.h")
#endif
    Q_PROPERTY(QString clusterName READ clusterName NOTIFY clusterNameChanged)
    Q_PROPERTY(QSize clusterSize READ clusterSize NOTIFY clusterSizeChanged)
    Q_PROPERTY(QPoint positionInCluster READ positionInCluster NOTIFY positionInClusterChanged)

public:
    enum AppMirroringState {
        AppMirroringStateInactive = 1,
        AppMirroringStateSender,
        AppMirroringStateReceiver,
        AppMirroringStateDisabled,
    };
    Q_ENUM(AppMirroringState);

    WebOSCompositorWindow(QString screenName = QString(), QString geometryString = QString(), QSurfaceFormat *surfaceFormat = 0);
    virtual ~WebOSCompositorWindow();

    static QList<WebOSCompositorWindow *> initializeExtraWindows(WebOSCoreCompositor* compositor, const int count, WebOSCompositorPluginLoader *pluginLoader = nullptr);
    static bool parseGeometryString(const QString string, QRect &geometry, int &rotation, double &ratio);
    // Testing purpose only
    static void resetDisplayCount();

    void setCompositor(WebOSCoreCompositor* compositor);
    bool setCompositorMain(const QUrl& main, const QString& importPath = QString());

    virtual void setPageFlipNotifier() {};
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

    void setOutput(QWaylandQuickOutput *output);
    QWaylandQuickOutput *output() { return m_output; }
    void setInputDevice(QWaylandSeat *device) { m_inputDevice = device; }
    QWaylandSeat *inputDevice() { return m_inputDevice; }
    WebOSSurfaceItem *appMirroringItem();
    void setAppMirroringItem(WebOSSurfaceItem *item);

    // Methods for App mirroring
    Q_INVOKABLE int startAppMirroring(int target);
    Q_INVOKABLE int stopAppMirroring(int target);
    Q_INVOKABLE int stopAppMirroringToAll(WebOSSurfaceItem *source = nullptr);
    Q_INVOKABLE int stopAppMirroringFromMirror(WebOSSurfaceItem *mirror);
    AppMirroringState appMirroringState() const { return m_appMirroringState; }
    void setAppMirroringState(AppMirroringState state);

    QString clusterName() const { return m_clusterName; }
    void setClusterName(QString name);
    QSize clusterSize() const { return m_clusterSize; }
    void setClusterSize(QSize size);
    QPoint positionInCluster() const { return m_positionInCluster; }
    void setPositionInCluster(QPoint position);

    QString modelString();

    bool hasSecuredContent();

    void setNextUpdate();
    void setNextUpdateWithDefaultNotifier();
    void deliverUpdateRequest();
    void reportSurfaceDamaged(WebOSSurfaceItem* const item);

private:
    void checkAdaptiveUpdate();
    void sendFrame();

    int stopAppMirroringInternal(WebOSSurfaceItem *source, WebOSSurfaceItem *mirror);

protected:
    virtual bool event(QEvent *) override;
    bool m_hasPageFlipNotifier = false;
    QString m_displayName;

    /* QQuickWindow does not support tablet events yet, so we implement it here.
     * WebOSCompositorWindow should deliver tablet event to a proper item(WebOSSurfaceItem)
     * on behalf of QQuickWindow. */
    bool handleTabletEvent(QQuickItem* item, QTabletEvent *);
    bool translateTabletToMouse(QTabletEvent* event, QQuickItem* item);

    WebOSCoreCompositor *compositor() const { return m_compositor; }

signals:
    void outputGeometryChanged();
    void outputRotationChanged();
    void outputClipChanged();
    void outputGeometryPendingChanged();
    void outputGeometryPendingIntervalChanged();

    void cursorVisibleChanged();
    void viewsRootChanged();
    void appMirroringItemChanged(WebOSSurfaceItem *);
    void appMirroringStateChanged();

    void accessibleChanged(const bool enabled);

    void clusterNameChanged();
    void clusterSizeChanged();
    void positionInClusterChanged();

    void pageFlipped();

private:
    // classes
    class RotationJob : public QRunnable
    {
    public:
        RotationJob(WebOSCompositorWindow* window) { m_window = window; }
        void run() override { m_window->sendOutputGeometry(); }
    private:
        WebOSCompositorWindow* m_window;
    };

    friend RotationJob;

    // methods
    void setNewOutputGeometry(QRect& outputGeometry, int outputRotation);
    void sendOutputGeometry() const;
    void applyOutputGeometry();

    // Keeps track of the item currently receiving mouse events
    QQuickItem *m_mouseGrabberItem;
    QQuickItem *m_tabletGrabberItem = nullptr;

private slots:
    void onOutputGeometryDone();
    void onOutputGeometryPendingExpired();
    void onAppMirroringItemChanged(WebOSSurfaceItem *oldItem);
    void onQmlError(const QList<QQmlError> &errors);
    void onBeforeSynchronizing();
    void onBeforeRendering();
    void onAfterRendering();
    void onPageFlipped();

private:
    // variables
    WebOSCoreCompositor* m_compositor;

    int m_displayId;

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
    QWaylandQuickOutput *m_output;
    QWaylandSeat *m_inputDevice;
    // auto clear on destroyed
    QPointer<WebOSSurfaceItem> m_appMirroringItem;
    AppMirroringState m_appMirroringState = AppMirroringStateInactive;

    QString m_clusterName;
    QSize m_clusterSize;
    QPoint m_positionInCluster;

    qreal m_vsyncInterval = 1.0 / 60 * 1000;

    bool m_adaptiveUpdate = false;
    QTimer m_updateTimer;
    bool m_hasUnhandledUpdateRequest = false;
    int m_updateTimerInterval = 0;

    bool m_adaptiveFrame = false;
    QElapsedTimer m_sinceSendFrame;
    QElapsedTimer m_sinceSyncStart;
    int m_timeSpentForRendering;
    bool m_waitForFlip = false;
    QTimer m_frameTimer;
    int m_frameTimerInterval = 0;
    QElapsedTimer m_sinceSurfaceDamaged;
};
#endif // WEBOSCOMPOSITORWINDOW_H
