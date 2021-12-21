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

#include <private/qguiapplication_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickwindow_p.h>

#include "weboscompositorwindow.h"
#include "weboscorecompositor.h"
#include "webossurfaceitem.h"
#include "weboscompositorpluginloader.h"
#include "weboscompositorconfig.h"
#include "weboscompositortracer.h"

static int s_displays = 0;

static constexpr int RENDER_FLUCTUATION_BUFFER_TIME = 4;
static constexpr int STATIC_SWAP_BUFFER_TIME        = 2;

inline __attribute__((always_inline)) static int calculateUpdateTimerInterval(
    int timeSinceRenderingToSwapBuffer, qreal vsyncInterval, int updateTimerInterval)
{
    int nextInterval = vsyncInterval - (timeSinceRenderingToSwapBuffer + RENDER_FLUCTUATION_BUFFER_TIME);
    nextInterval = nextInterval > 0 ? nextInterval : updateTimerInterval;
    updateTimerInterval = updateTimerInterval >= nextInterval ?
        nextInterval : updateTimerInterval + 1;
    return updateTimerInterval;
}

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
    , m_mouseGrabberItem(nullptr)
    , m_viewsRoot(nullptr)
    , m_timeSpentForRendering(0)
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
                // Needed in Qt 6 as otherwise it ends up with a crash
                setGeometry(screens.at(i)->geometry());
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

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    setClearBeforeRendering(true);
#endif
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
}

void WebOSCompositorWindow::checkAdaptiveUpdate()
{
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

    m_output->setAutomaticFrameCallback(!m_adaptiveFrame);

    if (m_adaptiveUpdate) {
        qInfo() << "Adaptive update interval for window" << this << "vsyncInterval:" << m_vsyncInterval;
        if (defaultUpdateInterval != 0)
            qWarning() << "QT_QPA_UPDATE_IDLE_TIME is not 0 but" << defaultUpdateInterval;
        m_updateTimerInterval = m_vsyncInterval / 2; // changes adaptively per every frame
        m_updateTimer.setTimerType(Qt::PreciseTimer);
        m_updateTimer.setSingleShot(true);

        connect(this, &QQuickWindow::beforeSynchronizing, this, &WebOSCompositorWindow::onBeforeSynchronizing);
        connect(this, &QQuickWindow::beforeRendering, this, &WebOSCompositorWindow::onBeforeRendering);
        if (!m_hasPageFlipNotifier) {
            connect(this, &QQuickWindow::afterRendering, this, &WebOSCompositorWindow::onAfterRendering);
            connect(this, &QQuickWindow::frameSwapped, this, &WebOSCompositorWindow::setNextUpdateWithDefaultNotifier);
        } else {
            connect(this, &QQuickWindow::frameSwapped, this, &WebOSCompositorWindow::setNextUpdate);
        }
        connect(this, &WebOSCompositorWindow::pageFlipped, this, &WebOSCompositorWindow::onPageFlipped);
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
    QRegularExpression re(QString::fromUtf8("([0-9]+)x([0-9]+)([\+\-][0-9]+)([\+\-][0-9]+)r([0-9]+)s([0-9]+\.?[0-9]*)"));
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

void WebOSCompositorWindow::resetDisplayCount()
{
    s_displays = 0;
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
    checkAdaptiveUpdate();

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
    QWindow *currentMouseWindow = QGuiApplicationPrivate::currentMouseWindow;
    if (currentMouseWindow && currentMouseWindow != static_cast<QWindow *>(this)) {
        qDebug() << "Cursor: ignore updateCursorFocus because it's not current mouse window:" << this << " current:" << currentMouseWindow;
        return;
    }

    if (cursorVisible()) {
        qDebug() << "Cursor: update cursor by sending a synthesized mouse event(visible case)";
        QPointF localPos = QPointF(mapFromGlobal(QCursor::pos()));
        QPointF globalPos = QPointF(QCursor::pos());
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
}

WebOSSurfaceItem *WebOSCompositorWindow::appMirroringItem()
{
    return m_appMirroringItem;
}

void WebOSCompositorWindow::setAppMirroringItem(WebOSSurfaceItem *item)
{
    if (m_appMirroringItem == item)
        return;

    WebOSSurfaceItem *oldItem = m_appMirroringItem;
    m_appMirroringItem = item;
    qInfo() << "App mirroring source item changed to:" << m_appMirroringItem << "for" << this << displayId();
    emit appMirroringItemChanged(oldItem);
}

void WebOSCompositorWindow::onAppMirroringItemChanged(WebOSSurfaceItem *oldItem)
{
    if (oldItem) {
        if (oldItem->isMirrorItem() && oldItem->mirrorSource())
            stopAppMirroringFromMirror(oldItem);
        else
            stopAppMirroringToAll(oldItem);
    }
}

void WebOSCompositorWindow::setAppMirroringState(AppMirroringState state)
{
    if (m_appMirroringState != state) {
        m_appMirroringState = state;
        emit appMirroringStateChanged();

        // Watch or unwatch appMirroringItem
        switch (m_appMirroringState) {
        case AppMirroringStateSender:
        case AppMirroringStateReceiver:
            connect(this, &WebOSCompositorWindow::appMirroringItemChanged, this, &WebOSCompositorWindow::onAppMirroringItemChanged);
            break;
        case AppMirroringStateInactive:
            disconnect(this, &WebOSCompositorWindow::appMirroringItemChanged, this, &WebOSCompositorWindow::onAppMirroringItemChanged);
            break;
        }
    }
}

int WebOSCompositorWindow::startAppMirroring(int target)
{
    WebOSCompositorWindow *tWindow = m_compositor->window(target);
    // No target window
    if (!tWindow)
        return -1;

    WebOSSurfaceItem *source = appMirroringItem();
    // No appMirroringItem
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
    // intended for App mirroring
    mirror->setDisplayAffinity(target);

    m_compositor->addSurfaceItem(mirror);
    // This should be after mapItem considering appMirroringItemChanged
    tWindow->setAppMirroringState(AppMirroringStateReceiver);
    setAppMirroringState(AppMirroringStateSender);

    return 0;
}

int WebOSCompositorWindow::stopAppMirroring(int targetId)
{
    WebOSSurfaceItem *source = appMirroringItem();
    if (!source) {
        qWarning() << "stopAppMirroring failed, no source item";
        return -1;
    }

    WebOSSurfaceItem *mirror = nullptr;
    foreach (WebOSSurfaceItem *m, source->mirrorItems()) {
        if (m->displayAffinity() == targetId) {
            mirror = m;
            break;
        }
    }
    if (!mirror) {
        qWarning() << "stopAppMirroring failed, no mirror item";
        return -1;
    }

    return stopAppMirroringInternal(source, mirror);
}

int WebOSCompositorWindow::stopAppMirroringToAll(WebOSSurfaceItem *source)
{
    WebOSSurfaceItem *sItem = source ? source : appMirroringItem();
    if (!sItem) {
        qWarning() << "stopAppMirroringToAll failed, no source item";
        return -1;
    }

    // Need to iterate with a copy as stopAppMirroringInternal alters the list
    QVector<WebOSSurfaceItem *> mirrors(sItem->mirrorItems());
    foreach (WebOSSurfaceItem *mirror, mirrors)
        stopAppMirroringInternal(sItem, mirror);

    return 0;
}

int WebOSCompositorWindow::stopAppMirroringFromMirror(WebOSSurfaceItem *mirror)
{
    if (!mirror->mirrorSource()) {
        qWarning() << "stopAppMirroringFromMirror failed, no source for given mirror item" << mirror;
        return -1;
    }

    WebOSCompositorWindow *sWindow = static_cast<WebOSCompositorWindow *>(mirror->mirrorSource()->window());
    if (!sWindow) {
        qWarning() << "stopAppMirroringFromMirror failed, mirror source doesn't belong to any window" << mirror << mirror->mirrorSource();
        return -1;
    }

    return sWindow->stopAppMirroringInternal(mirror->mirrorSource(), mirror);
}

int WebOSCompositorWindow::stopAppMirroringInternal(WebOSSurfaceItem *source, WebOSSurfaceItem *mirror)
{
    // Disregard the case where the mirroring state is not set
    // because stopAppMirroring should pair with startAppMirroring
    if (m_appMirroringState != AppMirroringStateSender)
        return 0;

    WebOSCompositorWindow *tWindow = static_cast<WebOSCompositorWindow *>(mirror->window());

    // Check various error cases
    if (!tWindow) {
        qWarning() << "stopAppMirroringInternal failed, no window for mirror item" << mirror;
        return -1;
    }
    if (static_cast<WebOSCompositorWindow *>(source->window()) != this) {
        qWarning() << "stopAppMirroringInternal failed, source" << source << "does not belong to" << this;
        return -1;
    }
    if (!source->mirrorItems().contains(mirror)) {
        qWarning() << "stopAppMirroringInternal failed, mirror item already removed from" << source;
        return -1;
    }

    source->removeMirrorItem(mirror);
    // Mirroring state goes to inactive only if there is no
    // mirror item with a valid displayAffinity.
    bool turnedToInactive = true;
    foreach (WebOSSurfaceItem *m, source->mirrorItems()) {
        if (m->displayAffinity() != -1) {
            turnedToInactive = false;
            break;
        }
    }
    if (turnedToInactive)
        setAppMirroringState(AppMirroringStateInactive);
    tWindow->setAppMirroringState(AppMirroringStateInactive);
    m_compositor->removeSurfaceItem(mirror, true);

    return 0;
}

void WebOSCompositorWindow::setClusterName(QString name)
{
    if (name != m_clusterName) {
        qDebug() << "Cluster name change for" << this << ":" << m_clusterName << "->" << name;
        m_clusterName = name;
        emit clusterNameChanged();
    }
}

void WebOSCompositorWindow::setClusterSize(QSize size)
{
    if (size != m_clusterSize) {
        qDebug() << "Cluster size change for" << this << ":" << m_clusterSize << "->" << size;
        m_clusterSize = size;
        emit clusterSizeChanged();
    }
}

void WebOSCompositorWindow::setPositionInCluster(QPoint position)
{
    if (position != m_positionInCluster) {
        qDebug() << "Position in cluster change for" << this << ":" << m_positionInCluster << "->" << position;
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

void WebOSCompositorWindow::onBeforeSynchronizing()
{
    PMTRACE_FUNCTION;
    m_sinceSyncStart.start();
}

void WebOSCompositorWindow::onBeforeRendering()
{
    PMTRACE_FUNCTION;
    m_waitForFlip = true;
}

void WebOSCompositorWindow::onAfterRendering()
{
    PMTRACE_FUNCTION;
    m_timeSpentForRendering = m_sinceSyncStart.elapsed();
}

void WebOSCompositorWindow::setNextUpdate()
{
    PMTRACE_FUNCTION;
    int timeSinceRenderingToSwapBuffer = m_sinceSyncStart.elapsed();
    m_updateTimerInterval =
        calculateUpdateTimerInterval(timeSinceRenderingToSwapBuffer,
                                     m_vsyncInterval, m_updateTimerInterval);
}

void WebOSCompositorWindow::setNextUpdateWithDefaultNotifier()
{
    PMTRACE_FUNCTION;
    // Assume that page flip happens just after the swapbuffer with the constant time
    int timeSinceRenderingToSwapBuffer = m_timeSpentForRendering + STATIC_SWAP_BUFFER_TIME;
    m_updateTimerInterval =
        calculateUpdateTimerInterval(timeSinceRenderingToSwapBuffer,
                                     m_vsyncInterval, m_updateTimerInterval);
    emit pageFlipped();
}

void WebOSCompositorWindow::onPageFlipped()
{
    PMTRACE_FUNCTION;
    m_updateTimer.start(m_updateTimerInterval);
    m_waitForFlip = false;
    if (m_adaptiveFrame)
        m_frameTimer.start(m_frameTimerInterval);
}

void WebOSCompositorWindow::deliverUpdateRequest()
{
    PMTRACE_FUNCTION;
    if (m_adaptiveUpdate) {
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

        if (m_sinceSurfaceDamaged.isValid()) {
            qint64 damageToSendFrameInterval = m_sinceSurfaceDamaged.elapsed();
            if (damageToSendFrameInterval > m_vsyncInterval * 2)
                qWarning() << "Elapsed since surface damaged until sending a frame callback:" << damageToSendFrameInterval << "ms";

            m_sinceSurfaceDamaged.invalidate();
        }
    }
}

void WebOSCompositorWindow::reportSurfaceDamaged(WebOSSurfaceItem* const item)
{
    Q_UNUSED(item);
    if (m_adaptiveFrame && m_sinceSendFrame.isValid()) {
        int nextFrameTime = (m_updateTimerInterval) - (m_sinceSendFrame.elapsed() + RENDER_FLUCTUATION_BUFFER_TIME);
        nextFrameTime = nextFrameTime < 0 ? 0 : nextFrameTime;
        // Decrease it immediately or increase it by 1
        m_frameTimerInterval = nextFrameTime <= m_frameTimerInterval
            ? nextFrameTime : m_frameTimerInterval + 1;
        m_sinceSendFrame.invalidate();

        m_sinceSurfaceDamaged.start();
    }
}

bool WebOSCompositorWindow::event(QEvent *e)
{
    PMTRACE_FUNCTION;

    // When the remaining time of QTimer is 0, it means that the timer has
    // expired but is being delayed due to other events and we need to
    // take actions needed right away.
    if (m_adaptiveUpdate && m_updateTimer.remainingTime() == 0) {
        m_updateTimer.stop();
        deliverUpdateRequest();
    }
    if (m_adaptiveFrame && m_frameTimer.remainingTime() == 0) {
        m_frameTimer.stop();
        sendFrame();
    }

    switch (e->type()) {
    case QEvent::UpdateRequest:
        if (m_adaptiveUpdate) {
            if (m_updateTimer.isActive() || m_waitForFlip) {
                m_hasUnhandledUpdateRequest = true;
                // The update timer may not be running in case there was no page flip
                // since the last update. So start the timer in that case as fallback
                // with the interval of two vsync cycles considering the worst case.
                if (!m_updateTimer.isActive())
                    m_updateTimer.start(m_vsyncInterval * 2);
                return true;
            }
            // First UpdateRequest, just fall through to start sync
        }
        break;
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        handleTabletEvent(QQuickWindowPrivate::get(this)->contentItem, static_cast<QTabletEvent *>(e));
        return true;
    default:
        break;
    }

    return QQuickWindow::event(e);
}

bool WebOSCompositorWindow::handleTabletEvent(QQuickItem* item, QTabletEvent* event)
{
    // Event grabber exists. Send it directly.
    if (m_tabletGrabberItem) {
        QPointF p = m_tabletGrabberItem->mapFromScene(event->posF());
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        QTabletEvent ev(event->type(), event->pointingDevice(), p, p,
                        event->pressure(),
                        event->xTilt(), event->yTilt(),
                        event->tangentialPressure(), event->rotation(),
                        event->z(), event->modifiers(),
                        event->button(), event->buttons());
#else
        QTabletEvent ev(event->type(), p, p, event->device(),
                        event->pointerType(), event->pressure(),
                        event->xTilt(), event->yTilt(),
                        event->tangentialPressure(), event->rotation(),
                        event->z(), event->modifiers(), event->uniqueId(),
                        event->button(), event->buttons());
#endif
        ev.accept();

        QCoreApplication::sendEvent(m_tabletGrabberItem, &ev);
        event->accept();

        if (event->type() == QEvent::TabletRelease)
            m_tabletGrabberItem = nullptr;
        return true;
    } else if (m_mouseGrabberItem) {
        return translateTabletToMouse(event, nullptr);
    }

    // Find the top-most item that can handle tablet events.
    // The main idea is inspired from QQuickWindowPrivate::deliverHoverEvent().
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(event->posF());
        if (!item->contains(p))
            return false;
    }

    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        if (!child->isVisible() || !child->isEnabled() || QQuickItemPrivate::get(child)->culled)
            continue;
        if (handleTabletEvent(child, event))
            return true;
    }

    QPointF p = item->mapFromScene(event->posF());

#ifdef WEBOS_TABLET_DEBUG
    qDebug() << "tablet item finding... tablet grabber:" << m_tabletGrabberItem <<
        " mouse grabber:" << m_mouseGrabberItem << " item:" << item << "=" <<
        item->contains(p) << "," << itemPrivate->acceptedMouseButtons();
#endif

    if (item->contains(p) && itemPrivate->acceptedMouseButtons()) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        QTabletEvent ev(event->type(), event->pointingDevice(), p, p,
                        event->pressure(),
                        event->xTilt(), event->yTilt(),
                        event->tangentialPressure(), event->rotation(),
                        event->z(), event->modifiers(),
                        event->button(), event->buttons());
#else
        QTabletEvent ev(event->type(), p, p, event->device(),
                        event->pointerType(), event->pressure(),
                        event->xTilt(), event->yTilt(),
                        event->tangentialPressure(), event->rotation(),
                        event->z(), event->modifiers(), event->uniqueId(),
                        event->button(), event->buttons());
#endif
        ev.accept();
        if (!m_mouseGrabberItem && QCoreApplication::sendEvent(item, &ev)) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
            if (event->pointerType() == QPointingDevice::PointerType::Pen && event->type() == QEvent::TabletRelease && event->pressure() != 0)
                m_tabletGrabberItem = item;
            else if (event->type() == QEvent::TabletPress)
                m_tabletGrabberItem = item;
#else
            if (event->pointerType() == QTabletEvent::Pen && event->type() == QEvent::TabletRelease && event->pressure() != 0)
                m_tabletGrabberItem = item;
            else if (event->type() == QEvent::TabletPress)
                m_tabletGrabberItem = item;
#endif
            event->accept();
            return true;
        } else {
            if (translateTabletToMouse(event, nullptr)) {
                event->accept();
                return true;
            }
        }
    }
    return false;
}

bool WebOSCompositorWindow::translateTabletToMouse(QTabletEvent* event, QQuickItem* item)
{
    Q_UNUSED(item);
    bool accepted = false;
    if  (event->type() == QEvent::TabletPress) {
        QMouseEvent mouseEvent(QEvent::MouseButtonPress, event->pos(),
                               Qt::LeftButton, Qt::LeftButton, event->modifiers());
        QQuickWindow::mousePressEvent(&mouseEvent);
        accepted = mouseEvent.isAccepted();
        m_mouseGrabberItem = mouseGrabberItem();
    } else if (event->type() == QEvent::TabletRelease) {
        QMouseEvent mouseEvent(QEvent::MouseButtonRelease, event->pos(),
                               Qt::LeftButton, Qt::NoButton, event->modifiers());
        QQuickWindow::mouseReleaseEvent(&mouseEvent);
        accepted = mouseEvent.isAccepted();
        m_mouseGrabberItem = nullptr;
    } else if (event->type() == QEvent::TabletMove) {
        QMouseEvent mouseEvent(QEvent::MouseMove, event->pos(),
                               Qt::LeftButton, Qt::LeftButton, event->modifiers());
        QQuickWindow::mouseMoveEvent(&mouseEvent);
        accepted = mouseEvent.isAccepted();
    }

    return accepted;
}

void WebOSCompositorWindow::onQmlError(const QList<QQmlError> &errors)
{
    qWarning("==== Exiting because of QML warnings ====");
    for (auto it = errors.constBegin(), end = errors.constEnd(); it != end; ++it)
        qWarning() << *it;
    qWarning("=========================================");
    QCoreApplication::exit(1);
}
