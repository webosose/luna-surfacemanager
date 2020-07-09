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

#include <QQmlContext>
#include <QQuickItem>
#include <QMetaObject>
#include <QGuiApplication>
#include <QScreen>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QStringList>
#include <QUrlQuery>

#include <qpa/qplatformscreen.h>

#include "weboscompositorwindow.h"
#include "weboscorecompositor.h"
#include "webossurfaceitem.h"
#include "weboscompositorpluginloader.h"
#include "weboscompositorconfig.h"
#include "weboscompositortracer.h"

static int s_displays = 0;

WebOSCompositorWindow::WebOSCompositorWindow(QString screenName, QString geometryString, QSurfaceFormat *surfaceFormat)
    : QQuickView()
    , m_compositor(0)
    , m_displayId(s_displays++)
    , m_baseGeometry(QRect(0, 0, 1920, 1080))
    , m_outputGeometry(QRect())
    , m_outputRotation(0)
    , m_outputClip(false)
    , m_outputRatio(1.0)
    , m_newOutputGeometry(QRect())
    , m_newOutputRotation(0)
    , m_newOutputRotationPending(-1)
    , m_outputGeometryPending(false)
    , m_outputGeometryPendingInterval(0)
    , m_cursorVisible(false)
    , m_output(nullptr)
    , m_inputDevice(nullptr)
{
    if (screenName.isEmpty()) {
        setScreen(QGuiApplication::primaryScreen());
        qInfo() << "Using displayId:" << m_displayId << "screen:" << screen() << "for this window" << this;
    } else {
        QList<QScreen *> screens = QGuiApplication::screens();
        qInfo() << "Screens:" << screens;
        for (int i = 0; i < screens.count(); i++) {
            if (screenName == screens.at(i)->handle()->name()) {
                setScreen(screens.at(i));
                qInfo() << "Setting displayId:" << m_displayId << "screen:" << screen() << "for this window" << this;
                break;
            }
        }
        if (!screen()) {
            setScreen(QGuiApplication::primaryScreen());
            qWarning() << "No screen named as" << screenName << ", trying to use the primary screen" << screen() << "with displayId" << m_displayId << "for this window" << this;
        }
    }

    m_displayName = screen()->name();
    setObjectName(m_displayName);

    if (surfaceFormat) {
        qInfo () << "Using surface format given:" << *surfaceFormat;
        setFormat(*surfaceFormat);
    } else {
        qInfo () << "Using default surface format:" << QWindow::format();
    }

    setClearBeforeRendering(true);
    setColor(Qt::transparent);

    // We need a platform window right now
    create();

    QSize screenSize = screen() ? screen()->size() : QSize();
    qreal dpr = devicePixelRatio();

    // Fallback if invalid
    if (!screenSize.isValid()) {
        screenSize.setWidth(1920);
        screenSize.setHeight(1080);
        qWarning() << "OutputGeometry:" << screen() << this << "screen size unset, use default" << screenSize;
    }
    if (dpr <= 0) {
        dpr = 1.0;
        qWarning() << "OutputGeometry:" << screen() << this << "device pixel ratio unset, use default" << dpr;
    }

    if (!geometryString.isEmpty()) {
        qInfo() << "OutputGeometry:" << screen() << "using geometryString" << geometryString;
    } else {
        // Refer to geometryString of the primary display
        geometryString = WebOSCompositorConfig::instance()->geometryString();
        qInfo() << "OutputGeometry:" << screen() << "using geometryString of primary display from config" << geometryString;
    }

    if (parseGeometryString(geometryString, m_outputGeometry, m_outputRotation, m_outputRatio)) {
        if (m_outputRotation % 90) {
            qWarning() << "OutputGeometry:" << screen() << this << "invalid output rotation from geometryString" << m_outputRotation << "fallback to 0";
            m_outputRotation = 0;
        }
        if (m_outputRatio <= 0) {
            qWarning() << "OutputGeometry:" << screen() << this << "invalid output ratio from geometryString" << m_outputRatio << "fallback to devicePixelRatio";
            m_outputRatio = (double) dpr;
        }
    } else {
        m_outputGeometry.setRect(0, 0, screenSize.width() / dpr, screenSize.height() / dpr);
        qWarning() << "OutputGeometry:" << screen() << this << "invalid geometry from geometryString, fallback to" << m_outputGeometry;
    }

    setResizeMode(QQuickView::SizeRootObjectToView);
    resize(screenSize / dpr);

    qInfo() << "OutputGeometry:" << screen() << this << "outputGeometry:" << m_outputGeometry << "outputRotation:" << m_outputRotation << "outputRatio:" << m_outputRatio;
    // More info
    qDebug() << "OutputGeometry:" << screen() << this << "screen size and dpr:" << screenSize << dpr;

    m_outputGeometryPendingTimer.setSingleShot(true);
    connect(&m_outputGeometryPendingTimer, &QTimer::timeout, this, &WebOSCompositorWindow::onOutputGeometryPendingExpired);
    if (WebOSCompositorConfig::instance()->exitOnQmlWarn())
        connect(engine(), &QQmlEngine::warnings, this, &WebOSCompositorWindow::onQmlError);

    // Start with cursor invisible
    invalidateCursor();

    // Get VSync interval
    m_vsyncInterval = 1.0 / screen()->refreshRate() * 1000;

    connect(this, &WebOSCompositorWindow::fullscreenItemChanged, &WebOSCompositorWindow::onFullscreenItemChanged);

    connect(this, &QQuickWindow::frameSwapped, this, &WebOSCompositorWindow::onFrameSwapped);

    static int defaultUpdateInterval = []() {
        bool ok = false;
        int customUpdateInterval = qEnvironmentVariableIntValue("QT_QPA_UPDATE_IDLE_TIME", &ok);
        return ok ? customUpdateInterval : 5;
    }();

    m_adaptiveUpdate = (qgetenv("WEBOS_COMPOSITOR_ADAPTIVE_UPDATE").toInt() == 1);
    m_adaptiveFrame = (qgetenv("WEBOS_COMPOSITOR_ADAPTIVE_FRAME_CALLBACK").toInt() == 1);

    // Adaptive frame callback requires adaptive update
    if (m_adaptiveFrame && !m_adaptiveUpdate)
        m_adaptiveFrame = false;

    if (m_adaptiveUpdate) {
        qInfo() << "Adaptive update interval for window" << this << "vsyncInterval:" << m_vsyncInterval;
        if (defaultUpdateInterval != 0)
            qWarning() << "QT_QPA_UPDATE_IDLE_TIME is not 0 but" << defaultUpdateInterval;
        m_updateTimerInterval = m_vsyncInterval / 2; // changes adaptively per every frame
        m_updateTimer.setTimerType(Qt::PreciseTimer);
        connect(&m_updateTimer, &QTimer::timeout, this, &WebOSCompositorWindow::deliverUpdateRequest);

        if (m_adaptiveFrame) {
            qInfo() << "Adaptive frame callback for window" << this;
            m_frameTimerInterval = m_vsyncInterval / 3; // changes adaptively per every frame
            m_frameTimer.setTimerType(Qt::PreciseTimer);
            m_frameTimer.setSingleShot(true);
            connect(&m_frameTimer, &QTimer::timeout, this, &WebOSCompositorWindow::sendFrame);
        } else {
            qInfo() << "Default frame callback for window" << this;
        }
    } else {
        qInfo() << "Default update interval" << defaultUpdateInterval << "for window" << this << "vsyncInterval:" << m_vsyncInterval;
    }
}

WebOSCompositorWindow::~WebOSCompositorWindow()
{
}

QList<WebOSCompositorWindow *> WebOSCompositorWindow::initializeExtraWindows(WebOSCoreCompositor* compositor, const int count, WebOSCompositorPluginLoader *pluginLoader)
{
    QList<WebOSCompositorWindow *> list;
    QList<QString> outputList = WebOSCompositorConfig::instance()->outputList();
    for (int i = 0; i < outputList.size(); i++) {
        QString outputName = outputList[i];
        QJsonObject outputConfig = WebOSCompositorConfig::instance()->outputConfigs().value(outputName);
        qDebug() << "output:" << outputName << outputConfig;
        if (outputName == WebOSCompositorConfig::instance()->primaryScreen()) {
            qInfo() << "ExtraWindow: skip primary" << outputName;
            continue;
        }
        WebOSCompositorWindow *extraWindow = nullptr;
        QString geometryString = outputConfig.value(QStringLiteral("geometry")).toString();
        if (geometryString.isEmpty())
            geometryString = WebOSCompositorConfig::instance()->geometryString();
        if (pluginLoader) {
            qInfo() << "ExtraWindow: trying the extended compositorWindow from the plugin" << outputName << geometryString;
            extraWindow = pluginLoader->compositorWindow(outputName, geometryString);
        }
        if (!extraWindow) {
            qInfo() << "ExtraWindow: using default WebOSCompositorWindow" << outputName << geometryString;
            extraWindow = new WebOSCompositorWindow(outputName, geometryString);
        }
        if (extraWindow) {
            // FIXME: Consider adding below if we need to call registerWindow
            // for an extra window
            //extraWindow->installEventFilter(new EventFilter(compositor));
            compositor->registerWindow(extraWindow, outputName);
            extraWindow->setCompositor(compositor);
            QUrl source = outputConfig.value(QStringLiteral("source")).toString();
            if (!source.isValid())
                source = WebOSCompositorConfig::instance()->source2();
            QString importPath = outputConfig.value(QStringLiteral("importPath")).toString();
            if (importPath.isEmpty())
                importPath = WebOSCompositorConfig::instance()->importPath();
            extraWindow->setCompositorMain(source, importPath);
            list.append(extraWindow);
            qInfo() << "ExtraWindow: an extra compositor window is added," << extraWindow << outputName << geometryString;
            if (list.size() >= count) {
                qInfo() << "ExtraWindow: created" << count << "extra compositor window(s) as expected";
                return list;
            }
        } else {
            qWarning() << "ExtraWindow: could not instantiate an extra compositor window for" << outputName << geometryString;
        }
    }

    return list;
}

bool WebOSCompositorWindow::parseGeometryString(const QString string, QRect &geometry, int &rotation, double &ratio)
{
    // Syntax: WIDTH[x]HEIGHT[+/-]X[+/-]Y[r]ROTATION[s]RATIO
    QRegularExpression re("([0-9]+)x([0-9]+)([\+\-][0-9]+)([\+\-][0-9]+)r([0-9]+)s([0-9]+\.?[0-9]*)");
    QRegularExpressionMatch match = re.match(string);

    if (match.hasMatch()) {
        QString w = match.captured(1);
        QString h = match.captured(2);
        QString x = match.captured(3);
        QString y = match.captured(4);
        QString r = match.captured(5);
        QString s = match.captured(6);
        geometry.setRect(x.toInt(), y.toInt(), w.toInt(), h.toInt());
        rotation = r.toInt();
        ratio = s.toDouble();
        qDebug() << "Geometry string" << string << "->" << w << h << x << y << r << s;
        return true;
    } else {
        qWarning() << "Invalid geometry string" << string;
    }

    return false;
}

void WebOSCompositorWindow::setCompositor(WebOSCoreCompositor* compositor)
{
    Q_ASSERT(!m_compositor);
    if (!m_compositor) {
        m_compositor = compositor;

        // Should be set after m_compositor is determined
        setBaseGeometry(m_outputGeometry, m_outputRotation, m_outputRatio);

        // Set global context properties available to qml everywhere
        rootContext()->setContextProperty(QLatin1String("compositor"), m_compositor);
        rootContext()->setContextProperty(QLatin1String("compositorWindow"), this);
    }
}

bool WebOSCompositorWindow::setCompositorMain(const QUrl& main, const QString& importPath)
{
    // Allow the source setting only once
    if (source().isValid()) {
        qCritical() << "Trying to override current source for window" << this;
        return false;
    }

    if (!main.isValid()) {
        qCritical() << this << "main QML" << main << "is not valid for window" << this;
        return false;
    }

    // Prepend import paths (important to keep the order)
    QStringList importPaths = engine()->importPathList();
    importPaths.prepend(QStringLiteral("qrc:/"));
    importPaths.prepend(WEBOS_INSTALL_QML "/WebOSCompositorBase/imports");
    importPaths.prepend(WEBOS_INSTALL_QML);
    if (!importPath.isEmpty())
        importPaths.prepend(importPath);
    engine()->setImportPathList(importPaths);
    qDebug() << "Import paths:" << importPaths;

    m_main = main;
    qInfo() << "Using main QML" << m_main << "for window" << this;

    if (m_compositor) {
        setSource(m_main);
        qInfo() << "Loaded main QML" << m_main << "for window" << this;
    } else {
        qWarning() << "No compositor assigned, will try to load" << m_main << "when showing the window" << this;
    }

    return true;
}

void WebOSCompositorWindow::showWindow()
{
    if (!source().isValid() && m_main.isValid()) {
        qInfo() << "Try to load main QML again" << m_main << "for window" << this;
        setSource(m_main);
    }

    // Set the default cursor of the root item
    if (rootObject())
        rootObject()->setCursor(QCursor(Qt::ArrowCursor));
    else
        qWarning() << this << "Root object is not set. Could not set the default cursor.";

    qInfo() << "Showing compositor window" << this;
    show();
}

void WebOSCompositorWindow::setAccessible(bool enable)
{
    if (m_accessible != enable) {
        qInfo() << "Accessible for window" << this << "enable: " << enable;
        m_accessible = enable;
        emit accessibleChanged(enable);
    }
}

QRect WebOSCompositorWindow::outputGeometry() const
{
    return m_outputGeometry.isValid() ? m_outputGeometry : m_baseGeometry;
}

int WebOSCompositorWindow::outputRotation() const
{
    return m_outputRotation;
}

bool WebOSCompositorWindow::outputClip() const
{
    return m_outputClip;
}

void WebOSCompositorWindow::setBaseGeometry(const QRect& baseGeometry, const int rotation, const double outputRatio)
{
    qInfo() << "OutputGeometry:" << this << "baseGeometry:" << m_baseGeometry << "->" << baseGeometry;
    m_baseGeometry = baseGeometry;

    qInfo() << "OutputGeometry:" << this << "outputRatio:" << m_outputRatio << "->" << outputRatio;
    m_outputRatio = outputRatio;

    bool clip = !m_baseGeometry.contains(geometry());
    if (m_outputClip != clip) {
        qInfo() << "OutputGeometry:" << this << "outputClip:" << m_outputClip << "->" << clip;
        m_outputClip = clip;
        emit outputClipChanged();
    }

    // Initial output update
    updateOutputGeometry(rotation, true);
}

void WebOSCompositorWindow::updateOutputGeometry(const int rotation, bool forced)
{
    if (Q_UNLIKELY(!m_compositor)) {
        qWarning() << this << "compositor is not set yet!";
        return;
    }

    if (rotation % 90) {
        qWarning() << "OutputGeometry:" << this << "invalid rotation value:" << rotation;
        return;
    }

    if (m_outputGeometryPending) {
        qWarning() << "OutputGeometry:" << this << "New rotation request while pending, we only use the last one. "
                   << rotation;
        m_newOutputRotationPending = rotation;
        return;
    }

    QRect newGeometry = outputGeometry();
    newGeometry.setWidth(rotation % 180 ? m_baseGeometry.height() : m_baseGeometry.width());
    newGeometry.setHeight(rotation % 180 ? m_baseGeometry.width() : m_baseGeometry.height());

    if (forced || m_outputGeometry != newGeometry || rotation != m_outputRotation) {
        setNewOutputGeometry(newGeometry, rotation);

        bool pending = false;
        if (m_outputGeometryPendingInterval != 0 && m_outputRotation % 180 != m_newOutputRotation % 180) {
            if (m_compositor->prepareOutputUpdate() > 0)
                pending = true;
        }

        if (pending) {
            setOutputGeometryPending(true);
            RotationJob* job = new RotationJob(this);
            scheduleRenderJob(job, QQuickWindow::AfterSwapStage);
            connect(m_compositor, &WebOSCoreCompositor::outputUpdateDone, this, &WebOSCompositorWindow::onOutputGeometryDone);
        } else {
            sendOutputGeometry();
            applyOutputGeometry();
        }
    }
}

void WebOSCompositorWindow::setNewOutputGeometry(QRect& outputGeometry, int outputRotation)
{
    m_newOutputGeometry = outputGeometry;
    m_newOutputRotation = outputRotation;
}

void WebOSCompositorWindow::sendOutputGeometry() const
{
    if (Q_UNLIKELY(!m_compositor)) {
        qWarning() << this << "compositor is not set yet!";
        return;
    }

    m_compositor->commitOutputUpdate(static_cast<QQuickWindow *>(const_cast<WebOSCompositorWindow *>(this)), m_newOutputGeometry, m_newOutputRotation, m_outputRatio);
}

void WebOSCompositorWindow::applyOutputGeometry()
{
    if (Q_UNLIKELY(!m_compositor)) {
        qWarning() << this << "compositor is not set yet!";
        return;
    }

    qInfo() << "OutputGeometry:" << this << "apply output geometry:" << m_outputGeometry << "->" << m_newOutputGeometry
        << "rotation:" << m_outputRotation << "->" << m_newOutputRotation;

    disconnect(m_compositor, &WebOSCoreCompositor::outputUpdateDone, this, &WebOSCompositorWindow::onOutputGeometryDone);

    if (m_outputGeometry != m_newOutputGeometry) {
        m_outputGeometry = m_newOutputGeometry;
        emit outputGeometryChanged();
    }

    if (m_outputRotation != m_newOutputRotation) {
        m_outputRotation = m_newOutputRotation;
        emit outputRotationChanged();
    }

    if (m_newOutputRotationPending != -1) {
        updateOutputGeometry(m_newOutputRotationPending, false);
        m_newOutputRotationPending = -1;
    }

    m_compositor->finalizeOutputUpdate();
}

void WebOSCompositorWindow::onOutputGeometryDone()
{
    // As some clients may need more time to update their surfaces,
    // use the timer to defer calling setOutputGeometryPending.
    qDebug() << "OutputGeometry:" << this << "all watched items have been resized";
    m_outputGeometryPendingTimer.start(200);
}

bool WebOSCompositorWindow::outputGeometryPending() const
{
    return m_outputGeometryPending;
}

void WebOSCompositorWindow::setOutputGeometryPending(bool pending)
{
    if (m_outputGeometryPending == pending)
        return;

    m_outputGeometryPending = pending;
    emit outputGeometryPendingChanged();

    if (m_outputGeometryPending) {
        qInfo() << "OutputGeometry:" << this
            << "pending update:" << m_outputGeometry << "->" << m_newOutputGeometry
            << "rotation:" << m_outputRotation << "->" << m_newOutputRotation
            << "timeout:" << m_outputGeometryPendingInterval;
        m_outputGeometryPendingTimer.start(m_outputGeometryPendingInterval);
    } else {
        m_outputGeometryPendingTimer.stop();
        applyOutputGeometry();
    }
}

int WebOSCompositorWindow::outputGeometryPendingInterval() const
{
    return m_outputGeometryPendingInterval;
}

void WebOSCompositorWindow::setOutputGeometryPendingInterval(int interval)
{
    m_outputGeometryPendingInterval = interval;
    emit outputGeometryPendingIntervalChanged();
}

void WebOSCompositorWindow::onOutputGeometryPendingExpired()
{
    qInfo() << "OutputGeometry:" << this << "timer expired";
    setOutputGeometryPending(false);
}

void WebOSCompositorWindow::setDefaultCursor()
{
    /* Qt::ArrowCursor means system default cursor */
    qDebug() << "Cursor: set the default cursor for" << this;
    setCursor(QCursor(Qt::ArrowCursor));
}

void WebOSCompositorWindow::invalidateCursor()
{
    qDebug() << "Cursor:" << this << "make cursor transparent and invalidate cursor item";

    static QCursor transparentCursor;
    static bool set = false;
    if (Q_UNLIKELY(!set)) {
        QPixmap p(1, 1);
        p.fill(Qt::transparent);
        transparentCursor = QCursor(p);
        set = true;
    }

    setCursor(transparentCursor);
}

void WebOSCompositorWindow::setCursorVisible(bool visibility)
{
    if (m_cursorVisible != visibility) {
        m_cursorVisible = visibility;
        emit cursorVisibleChanged();
        if (!m_cursorVisible && cursor().shape() != Qt::BlankCursor)
            invalidateCursor();
    }
}

/* Compositor should call this whenever system UI shows/disappears
 * to restore cursor shape without mouse move */
void WebOSCompositorWindow::updateCursorFocus(Qt::KeyboardModifiers modifiers)
{
    if (cursorVisible()) {
        qDebug() << "Cursor: update cursor by sending a synthesized mouse event(visible case)";
        QPointF localPos = QPointF(QCursor::pos());
        QPointF globalPos = QPointF(mapToGlobal(QCursor::pos()));
        QMouseEvent *move = new QMouseEvent(QEvent::MouseMove,
                localPos, localPos, globalPos,
                Qt::NoButton, Qt::NoButton, modifiers,
                Qt::MouseEventSynthesizedByApplication);
        QCoreApplication::postEvent(this, move);
    } else {
        qDebug() << "Cursor: let cursor be updated by upcoming event(invisible case)";
        invalidateCursor();
    }
}

void WebOSCompositorWindow::setViewsRoot(QQuickItem *viewsRoot)
{
    if (m_viewsRoot != viewsRoot) {
        m_viewsRoot = viewsRoot;
        emit viewsRootChanged();
    }
}

void WebOSCompositorWindow::setOutput(QWaylandQuickOutput *output)
{
    m_output = output;
    m_output->setAutomaticFrameCallback(!m_adaptiveFrame);
}

WebOSSurfaceItem *WebOSCompositorWindow::fullscreenItem()
{
    return m_fullscreenItem;
}

void WebOSCompositorWindow::setFullscreenItem(WebOSSurfaceItem *item)
{
    if (m_fullscreenItem == item)
        return;

    WebOSSurfaceItem *oldItem = m_fullscreenItem;
    m_fullscreenItem = item;
    qInfo() << "Fullscreen item changed to:" << m_fullscreenItem << "for" << this << displayId();
    emit fullscreenItemChanged(oldItem);
}

void WebOSCompositorWindow::onFullscreenItemChanged(WebOSSurfaceItem *oldItem)
{
    if (oldItem) {
        if (oldItem->isMirrorItem() && oldItem->mirrorSource())
            stopMirroringFromMirror(oldItem);
        else
            stopMirroringToAll(oldItem);
    }
}

void WebOSCompositorWindow::setMirroringState(MirroringState state)
{
    if (m_mirrorState != state) {
        m_mirrorState = state;
        emit mirroringStateChanged();
    }
}

int WebOSCompositorWindow::startMirroring(int target)
{
    WebOSCompositorWindow *tWindow = m_compositor->window(target);
    // No target window
    if (!tWindow)
        return -1;

    WebOSSurfaceItem *source = fullscreenItem();
    // No fullscreenItem
    if (!source)
        return -1;

    WebOSSurfaceItem *mirror = source->createMirrorItem();
    if (!mirror) {
        qWarning("Creating mirror item failed, should not happen");
        return -1;
    }

    // Get the mirror item like a normal surface item in the target window
    mirror->setAppId(source->appId());
    mirror->setType(source->type());
    // It is important to set the correct displayAffinity for the mirror item
    // intended for display-to-display mirroring
    mirror->setDisplayAffinity(target);

    m_compositor->addSurfaceItem(mirror);
    // This should be after mapItem considering fullscreenItemChanged
    tWindow->setMirroringState(MirroringStateReceiver);
    setMirroringState(MirroringStateSender);

    return 0;
}

int WebOSCompositorWindow::stopMirroring(int targetId)
{
    WebOSSurfaceItem *source = fullscreenItem();
    if (!source)
        return -1;

    WebOSSurfaceItem *mirror = nullptr;
    foreach (WebOSSurfaceItem *m, source->mirrorItems()) {
        if (m->displayAffinity() == targetId) {
            mirror = m;
            break;
        }
    }
    if (!mirror) {
        qWarning("No mirror item, should not happen");
        return -1;
    }

    return stopMirroringInternal(source, mirror);
}

int WebOSCompositorWindow::stopMirroringToAll(WebOSSurfaceItem *source)
{
    WebOSSurfaceItem *sItem = source ? source : fullscreenItem();
    if (!sItem)
        return -1;

    // Need to iterate with a copy as stopMirroringInternal alters the list
    QVector<WebOSSurfaceItem *> mirrors(sItem->mirrorItems());
    foreach (WebOSSurfaceItem *mirror, mirrors)
        stopMirroringInternal(sItem, mirror);

    return 0;
}

int WebOSCompositorWindow::stopMirroringFromMirror(WebOSSurfaceItem *mirror)
{
    if (!mirror->mirrorSource()) {
        qWarning() << "No mirror source" << mirror;
        return -1;
    }

    WebOSCompositorWindow *sWindow = static_cast<WebOSCompositorWindow *>(mirror->mirrorSource()->window());
    if (!sWindow) {
        qWarning() << "Mirror source doesn't belong to any window" << mirror << mirror->mirrorSource();
        return -1;
    }

    return sWindow->stopMirroringInternal(mirror->mirrorSource(), mirror);
}

int WebOSCompositorWindow::stopMirroringInternal(WebOSSurfaceItem *source, WebOSSurfaceItem *mirror)
{
    WebOSCompositorWindow *tWindow = static_cast<WebOSCompositorWindow *>(mirror->window());

    if (!tWindow) {
        qWarning("No target, should not happen");
        return -1;
    }

    source->removeMirrorItem(mirror);
    // Mirroring state goes to inactive only if there is no
    // mirror item with a valid displayAffinity.
    bool turnedToInactive = true;
    foreach (WebOSSurfaceItem *mirror, source->mirrorItems()) {
        if (mirror->displayAffinity() != -1) {
            turnedToInactive = false;
            break;
        }
    }
    if (turnedToInactive)
        setMirroringState(MirroringStateInactive);
    tWindow->setMirroringState(MirroringStateInactive);
    m_compositor->removeSurfaceItem(mirror, true);

    return 0;
}

void WebOSCompositorWindow::setPositionInCluster(QPoint position)
{
    if (position != m_positionInCluster) {
        qDebug() << "Position in cluster change for" << this << ":" << position << "->" << m_positionInCluster;
        m_positionInCluster = position;
        emit positionInClusterChanged();
    }
}

QString WebOSCompositorWindow::modelString()
{
    // This is supposed to be used for "model" of the corresponding wl_output.
    // We carry some meaningful display information through this.
    QUrlQuery qs;
    qs.addQueryItem(QLatin1String("display_id"), QString::number(m_displayId));
    qs.addQueryItem(QLatin1String("name"), m_displayName);

    return qs.toString();
}

bool WebOSCompositorWindow::hasSecuredContent()
{
    if (Q_UNLIKELY(!m_compositor)) {
        qWarning() << this << "compositor is not set yet!";
        return false;
    }

    foreach (WebOSSurfaceItem *item, m_compositor->getItems(this)) {
        if (item && item->hasSecuredContent())
            return true;
    }

    return false;
}

void WebOSCompositorWindow::onFrameSwapped()
{
    PMTRACE_FUNCTION;
    if (m_adaptiveUpdate) {
        if (m_sinceFrameSwapped.isValid()) {
            int sinceLastFrameSwapped = m_sinceFrameSwapped.elapsed();
            int nextDeliverUpdateTime = m_updateTimerInterval + (int)(m_vsyncInterval - sinceLastFrameSwapped);
            // Use existing values if the frame is not continuous ( sinceLastFrameSwapped > 32ms ).
            nextDeliverUpdateTime = (nextDeliverUpdateTime <= 0 || nextDeliverUpdateTime >= m_vsyncInterval)
                ? m_updateTimerInterval : nextDeliverUpdateTime;
            m_updateTimerInterval = nextDeliverUpdateTime >= m_updateTimerInterval
                ? nextDeliverUpdateTime : m_updateTimerInterval - 1;
        }
        m_updatesSinceFrameSwapped = 0;
        m_sinceFrameSwapped.start();
        m_updateTimer.start(m_updateTimerInterval);
        if (m_adaptiveFrame)
            m_frameTimer.start(m_frameTimerInterval);
    }
}

void WebOSCompositorWindow::deliverUpdateRequest()
{
    PMTRACE_FUNCTION;
    if (m_adaptiveUpdate) {
        qreal correctionValue = 0;

        correctionValue = m_sinceFrameSwapped.elapsed() - m_vsyncInterval * (++m_updatesSinceFrameSwapped);
        if (correctionValue > m_vsyncInterval) {
            // Case that the timer event arrives after a vsync interval
            correctionValue = 0;
        }

        // Calibrate the update timer interval
        m_updateTimer.start(m_vsyncInterval - correctionValue);
        if (m_updatesSinceFrameSwapped >= 10) {
            m_sinceFrameSwapped.start();
            m_updatesSinceFrameSwapped = 0;
        }

        // Send any unhandled UpdateRequest
        if (m_hasUnhandledUpdateRequest) {
            QEvent request(QEvent::UpdateRequest);
            static_cast<void>(QQuickWindow::event(&request));
            m_hasUnhandledUpdateRequest = false;
        }
    } else {
        qWarning() << "deliverUpdateRequest is called for" << this << "while adaptiveUpdate is NOT set!";
    }
}

void WebOSCompositorWindow::sendFrame()
{
    PMTRACE_FUNCTION;
    if (m_adaptiveFrame && m_output) {
        if (m_sinceSendFrame.isValid()) {
            // No reportSurfaceDamaged called since the last frame.
            // It means that rendering time on client exceeds the vsync interval.
            m_frameTimerInterval = 0;
        }
        m_sinceSendFrame.start();
        m_output->sendFrameCallbacks();
    }
}

void WebOSCompositorWindow::reportSurfaceDamaged(WebOSSurfaceItem* const item)
{
    Q_UNUSED(item);
    if (m_adaptiveFrame && m_sinceSendFrame.isValid()) {
        int nextFrameTime = (m_updateTimerInterval) - (m_sinceSendFrame.elapsed() + 4);
        nextFrameTime = nextFrameTime < 0 ? 0 : nextFrameTime;
        // Decrease it immediately or increase it by 1
        m_frameTimerInterval = nextFrameTime <= m_frameTimerInterval
            ? nextFrameTime : m_frameTimerInterval + 1;
        m_sinceSendFrame.invalidate();
    }
}

bool WebOSCompositorWindow::event(QEvent *e)
{
    PMTRACE_FUNCTION;
    switch (e->type()) {
    case QEvent::UpdateRequest:
        if (m_adaptiveUpdate) {
            if (m_updateTimer.isActive()) {
                m_hasUnhandledUpdateRequest = true;
                return true;
            }
            // First UpdateRequest, just fall through to start sync
        }
        break;
    default:
        break;
    }

    return QQuickWindow::event(e);
}

void WebOSCompositorWindow::onQmlError(const QList<QQmlError> &errors)
{
    qWarning("==== Exiting because of QML warnings ====");
    for (auto it = errors.constBegin(), end = errors.constEnd(); it != end; ++it)
        qWarning() << *it;
    qWarning("=========================================");
    QCoreApplication::exit(1);
}
