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
    , m_mirrorSource(nullptr)
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

    connect(this, &WebOSCompositorWindow::fullscreenItemChanged, &WebOSCompositorWindow::onFullscreenItemChanged);
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
            extraWindow->setCompositorMain(source);
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

bool WebOSCompositorWindow::setCompositorMain(const QUrl& main)
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
    m_compositor->finalizeOutputUpdate();

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
    invalidateCursorItem();
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

void WebOSCompositorWindow::setViewsRoot(QQuickItem *viewsRoot) {
    if (m_viewsRoot != viewsRoot) {
        m_viewsRoot = viewsRoot;
        emit viewsRootChanged();
    }
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

void WebOSCompositorWindow::updateForegroundItems(QList<QObject *> items)
{
    m_foregroundItems = items;

    if (Q_UNLIKELY(!m_compositor)) {
        qWarning() << this << "compositor is not set yet!";
        return;
    }

    emit m_compositor->foregroundItemsChanged();
}

void WebOSCompositorWindow::onFullscreenItemChanged(WebOSSurfaceItem *oldItem)
{
    if (stopMirroringFromTarget() == 0)
        return;

    stopMirroringToAll(oldItem);
}

QVector<bool> WebOSCompositorWindow::isMirroringTo()
{
    QVector<bool> isMirroring(m_compositor->windows().size(), false);

    if (fullscreenItem()) {
        QList<int> targets = fullscreenItem()->mirrorTargetIds();
        for (int i = 0; i < targets.size(); i++)
            isMirroring[targets[i]] = true;
    }

    return isMirroring;
}

bool WebOSCompositorWindow::hasMirrorSource() const
{
    return m_mirrorSource != nullptr;
}

void WebOSCompositorWindow::setMirrorSource(WebOSCompositorWindow *window)
{
    Q_ASSERT(!m_mirrorSource || !window);

    m_mirrorSource = window;

    if (m_mirrorSource)
        setMirroringState(MirroringStateReceiver);
    else
        setMirroringState(MirroringStateInactive);
}

void WebOSCompositorWindow::setMirroringState(MirroringState state)
{
    if (m_mirrorState != state) {
        m_mirrorState = state;
        emit mirroringStateChanged();
    }
}

// TODO: Error codes should be specified
int WebOSCompositorWindow::startMirroring(int target)
{
    WebOSCompositorWindow *tWindow = m_compositor->window(target);
    // No target Window or already mirrored
    if (!tWindow || tWindow->hasMirrorSource() || hasMirrorSource())
        return -1;

    WebOSSurfaceItem *sItem = fullscreenItem();
    // No fullscreenItem
    if (!sItem)
        return -1;

    WebOSSurfaceItem *mirror = sItem->createMirrorItem(target);
    if (!mirror) {
        qWarning("Creating mirror item failed, should not happen");
        return -1;
    }

    m_compositor->addSurfaceItem(mirror);
    // This should be after mapItem considering fullscreenItemChanged
    tWindow->setMirrorSource(this);
    setMirroringState(MirroringStateSender);
    emit isMirroringToChanged();

    return 0;
}

int WebOSCompositorWindow::stopMirroringFromTarget()
{
    if (!hasMirrorSource())
        return -1;

    return  mirrorSource()->stopMirroring(displayId());
}

// TODO: Error codes should be specified for belows
int WebOSCompositorWindow::stopMirroringInternal(WebOSSurfaceItem *sItem, int targetId)
{
    Q_ASSERT(sItem);

    WebOSSurfaceItem *mirror = sItem->takeMirrorItem(targetId);
    WebOSCompositorWindow *tWindow = m_compositor->window(targetId);
    if (!mirror) {
        qWarning("No mirror item, should not happen");
        return -1;
    }

    if (!tWindow || !tWindow->hasMirrorSource()) {
        qWarning("No window or not mirrored, should not happen");
        return -1;
    }

    if (sItem->mirrorTargetIds().size() == 0)
        setMirroringState(MirroringStateInactive);
    tWindow->setMirrorSource(nullptr);
    m_compositor->removeSurfaceItem(mirror, true);
    emit isMirroringToChanged();

    return 0;
}

int WebOSCompositorWindow::stopMirroring(int targetId)
{
    WebOSSurfaceItem *sItem = fullscreenItem();
    if (!sItem)
        return -1;

    return stopMirroringInternal(sItem, targetId);
}

int WebOSCompositorWindow::stopMirroringToAll(WebOSSurfaceItem *source)
{
    WebOSSurfaceItem *sItem = source ? source : fullscreenItem();
    if (!sItem)
        return -1;

    QList<int> targetIds = sItem->mirrorTargetIds();
    for (int i = 0; i < targetIds.size(); i++) {
        // TODO: Need to check return value
        stopMirroringInternal(sItem, targetIds[i]);
    }

    return 0;
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

void WebOSCompositorWindow::onQmlError(const QList<QQmlError> &errors)
{
    qWarning("==== Exiting because of QML warnings ====");
    for (auto it = errors.constBegin(), end = errors.constEnd(); it != end; ++it)
        qWarning() << *it;
    qWarning("=========================================");
    QCoreApplication::exit(1);
}
