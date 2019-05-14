// Copyright (c) 2013-2020 LG Electronics, Inc.
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

#include "webossurfaceitem.h"
#include "webossurfacegroup.h"
#include "weboswindowmodel.h"
#include "webosgroupedwindowmodel.h"
#include "webossurfacemodel.h"
#include "weboscorecompositor.h"
#include "weboscompositorwindow.h"
#include "weboscompositortracer.h"
#include "webosshellsurface.h"
#include "webosinputmethod.h"
#include "webosforeign.h"
#include "webosevent.h"
#ifdef MULTIINPUT_SUPPORT
#include "webosinputdevice.h"
#endif
#include <QDateTime>
#include <QQmlEngine>
#include <QOpenGLTexture>
#include <QDebug>

#include <qweboskeyextension.h>

#include <QtCompositor/qwaylandbufferref.h>
#include <QtCompositor/private/qwlinputdevice_p.h>
#include <QtCompositor/private/qwlkeyboard_p.h>
#include <QtCompositor/private/qwlpointer_p.h>

#include "weboscompositortracer.h"

WebOSSurfaceItem::WebOSSurfaceItem(WebOSCoreCompositor* compositor, QWaylandQuickSurface* surface)
        : QWaylandSurfaceItem(surface)
        , m_compositor(compositor)
        , m_fullscreen(false)
        , m_lastFullscreenTick(0)
        , m_transientModel(0)
        , m_groupedWindowModel(0)
        , m_cardSnapShotFilePath()
        , m_customImageFilePath(QLatin1String("none"))
        , m_backgroundImageFilePath()
        , m_backgroundColor("")
        , m_shellSurface(0)
        , m_itemState(ItemStateNormal)
        , m_notifyPositionToClient(true)
        , m_displayId(-1)
        , m_displayAffinity(0)
        , m_appId("")
        , m_type("_WEBOS_WINDOW_TYPE_CARD")
        , m_windowClass(WindowClass_Normal)
        , m_title("")
        , m_subtitle("")
        , m_params("")
        , m_processId()
        , m_exposed(false)
        , m_launchRequired(false)
        , m_surfaceGroup(0)
        , m_hasKeyboardFocus(false)
        , m_grabKeyboardFocusOnClick(true)
        , m_closePolicy(QVariantMap())
        , m_itemStateReason(QString())
        , m_launchLastApp(false)
{
    if (surface) {
        connect(surface, SIGNAL(damaged(const QRegion &)), this, SLOT(onSurfaceDamaged(const QRegion &)));
    }

    connect(this, &QQuickItem::xChanged, this, &WebOSSurfaceItem::updateScreenPosition);
    connect(this, &QQuickItem::yChanged, this, &WebOSSurfaceItem::updateScreenPosition);
    // Set the ownership as CppOwnership explicitly to prevent from garbage collecting by JS engine
    QQmlEngine::setObjectOwnership((QObject*)this, QQmlEngine::CppOwnership);

    //In Qt5.2, All MouseAreas have default cursor as default value. So all surfaceitems should have default cursor as a initial value so that
    //it can restore the cursor from the system ui's cursor. See QQuickWindowPrivate::updateCursor()
    setCursor(Qt::ArrowCursor);

    if (!qgetenv("WEBOS_DISABLE_TOUCH").isEmpty())
        setTouchEventsEnabled(false);
    else
        setTouchEventsEnabled(true);

    connect(this, &QQuickItem::windowChanged, this, &WebOSSurfaceItem::handleWindowChanged);
}

WebOSSurfaceItem::~WebOSSurfaceItem()
{
    emit surfaceAboutToBeDestroyed();

    sendCloseToGroupItems();
    if (isSurfaceGroupRoot())
        m_surfaceGroup->setRootItem(NULL);
    m_surfaceGroup = NULL;
    deleteSnapShot();
    delete m_shellSurface;

    if (m_mirrorItems.size() > 0)
        qCritical() << "Zombie mirror items, should not happen";
}

void WebOSSurfaceItem::setDisplayId(int id)
{
    if (m_displayId != id) {
        m_displayId = id;
        emit displayIdChanged();
    }
}

void WebOSSurfaceItem::setDisplayAffinity(int affinity)
{
    if (m_displayAffinity != affinity) {
        qInfo() << "setting display affinity" << affinity << "for" << this;
        m_displayAffinity = affinity;
        emit displayAffinityChanged();
        emit dataChanged();
    }
}

void WebOSSurfaceItem::handleWindowChanged()
{
    qInfo() << this << "moved to window" << window();
    setDisplayId(window() ? static_cast<WebOSCompositorWindow *>(window())->displayId() : -1);
}

void WebOSSurfaceItem::requestMinimize()
{
    PMTRACE_FUNCTION;
    emit m_compositor->minimizeRequested(this);
}

bool WebOSSurfaceItem::requestFullscreen()
{
    PMTRACE_FUNCTION;
    emit m_compositor->fullscreenRequested(this);
    // TODO remove return types
    return true;
}

bool WebOSSurfaceItem::fullscreen() const
{
    return m_fullscreen;
}

void WebOSSurfaceItem::setFullscreen(bool enabled)
{
    PMTRACE_FUNCTION;
    // Currently this should not be called directly from qml or from other
    // place. To make a surface full screen use "requestFullscreen()" method
    if (m_fullscreen != enabled) {
        m_fullscreen = enabled;
        emit fullscreenChanged(m_fullscreen);
        if (enabled) {
            setExposed(true);
        } else {
            // Update the tick when the item goes into recents
            // Otherwise, a Card which is just launched will be sorted during animation.
            m_lastFullscreenTick = m_compositor->getFullscreenTick();
            emit lastFullscreenTickChanged();
        }

        emit dataChanged();
    }
}

QPointF WebOSSurfaceItem::mapToTarget(const QPointF& point) const
{
    if (!surface()) {
        // In the case we do not have a surface, i.e. mem manager has asked us to
        // kill it and pointer events end up here when in recents...
        return point;
    }
    qreal iWidth = width();
    qreal iHeight = height();
    int sWidth = surface()->size().width();
    int sHeight = surface()->size().height();

    if ((int)iWidth == sWidth && (int)iHeight == sHeight) {
        return point;
    }
    return QPointF((sWidth / iWidth) * point.x(), (sHeight / iHeight) * point.y());
}

QList<QTouchEvent::TouchPoint> WebOSSurfaceItem::mapToTarget(const QList<QTouchEvent::TouchPoint>& points) const
{
    QList<QTouchEvent::TouchPoint> result;
    foreach (QTouchEvent::TouchPoint point, points) {
        point.setPos(mapToTarget(point.pos()));
        result.append(point);
    }
    return result;
}

void WebOSSurfaceItem::takeWlKeyboardFocus() const
{
    if (!isSurfaced()) {
        qWarning("null surface(), not setting focus");
        return;
    }
#ifdef MULTIINPUT_SUPPORT
    /* set keyboard focus for all devices */
    foreach (QWaylandInputDevice *dev, m_compositor->inputDevices()) {
        if (dev)
            dev->setKeyboardFocus(surface());
    }
#else
    /* set keyboard focus for this item */
    QWaylandInputDevice *dev = m_compositor->keyboardDeviceForWindow(window());
    if (dev)
        dev->setKeyboardFocus(surface());
#endif
}

bool WebOSSurfaceItem::contains(const QPointF& point) const
{
    return surface() && surface()->inputRegionContains(mapToTarget(point).toPoint());
}

bool WebOSSurfaceItem::isMapped()
{
    return m_compositor && m_compositor->isMapped(this);
}

void WebOSSurfaceItem::hoverMoveEvent(QHoverEvent *event)
{
    if (acceptHoverEvents()) {
        QMouseEvent e(QEvent::MouseMove, event->pos(), Qt::NoButton, Qt::NoButton, event->modifiers());
        mouseMoveEvent(&e);
    }
}

void WebOSSurfaceItem::mouseMoveEvent(QMouseEvent * event)
{
    WebOSMouseEvent e(event->type(), mapToTarget(QPointF(event->pos())).toPoint(),
                  event->button(), event->buttons(), event->modifiers(), window());
    QWaylandSurfaceItem::mouseMoveEvent(&e);
}

void WebOSSurfaceItem::mousePressEvent(QMouseEvent *event)
{
    WebOSMouseEvent e(event->type(), mapToTarget(event->localPos()).toPoint(),
                  event->button(), event->buttons(), event->modifiers(), window());

    if (surface()) {
        WebOSCompositorWindow *w = static_cast<WebOSCompositorWindow *>(window());
#ifdef MULTIINPUT_SUPPORT
        QWaylandInputDevice *inputDevice = getInputDevice(&e);
        QWaylandInputDevice *keyboardDevice = inputDevice;
#else
        QWaylandInputDevice *inputDevice = w->inputDevice();
        QWaylandInputDevice *keyboardDevice = getInputDevice();
#endif
        if (inputDevice && keyboardDevice) {
            if (inputDevice->mouseFocus() != this)
                inputDevice->setMouseFocus(this, e.localPos(), e.windowPos());

            if (inputDevice->mouseFocus()
                    && inputDevice->mouseFocus()->surface() != keyboardDevice->keyboardFocus()
                    && m_grabKeyboardFocusOnClick) {
                takeWlKeyboardFocus();
                m_hasKeyboardFocus = true;
                emit hasKeyboardFocusChanged();
            }

            if (!w->accessible()) {
                inputDevice->sendMousePressEvent(e.button(), e.localPos(), e.windowPos());
            } else {
                // In accessibility mode there should be no extra mouse move event sent.
                // That is why we call another version of sendMousePressEvent here
                // which sends a button event only.
                inputDevice->handle()->pointerDevice()->setMouseFocus(this, e.localPos(), e.windowPos());
                inputDevice->handle()->pointerDevice()->sendMousePressEvent(e.button());
            }
        } else {
            qWarning() << "no input device for this event";
        }
    }
}

void WebOSSurfaceItem::mouseReleaseEvent(QMouseEvent *event)
{
    WebOSMouseEvent e(event->type(), mapToTarget(event->localPos()).toPoint(),
                  event->button(), event->buttons(), event->modifiers(), window());

    WebOSCompositorWindow *w = static_cast<WebOSCompositorWindow *>(window());

    if (!w->accessible()) {
        QWaylandSurfaceItem::mouseReleaseEvent(&e);
    } else {
        // In accessibility mode there should be no extra mouse move event sent.
        // That is why we call another version of sendMousePressEvent here
        // which sends a button event only.
        QWaylandInputDevice *inputDevice = w->inputDevice();
        if (inputDevice) {
            inputDevice->handle()->pointerDevice()->setMouseFocus(this, e.localPos(), e.windowPos());
            inputDevice->handle()->pointerDevice()->sendMouseReleaseEvent(e.button());
        } else {
            qWarning() << "no input device for this event";
        }
    }
}

void WebOSSurfaceItem::wheelEvent(QWheelEvent *event)
{
    WebOSWheelEvent e(mapToTarget(event->pos()), event->globalPos(), event->pixelDelta(),
                  event->angleDelta(), event->delta(), event->orientation(),
                  event->buttons(), event->modifiers(), event->phase(), window());

    m_compositor->setMouseFocus(surface());

    QWaylandSurfaceItem::wheelEvent(&e);
}

void WebOSSurfaceItem::touchEvent(QTouchEvent *event)
{
    QTouchEvent e(event->type(), event->device(), event->modifiers(),
                  event->touchPointStates(), mapToTarget(event->touchPoints()));
    e.setWindow(event->window());

    // This may not be needed with QtWayland 5.12
    // Currently, this is due to QWaylandSurfaceItem::mouseUngrabEvent
    // which sends the Cancel without window().
    if (!event->window())
        e.setWindow(window());

    QWaylandSurfaceItem::touchEvent(&e);
}

void WebOSSurfaceItem::hoverEnterEvent(QHoverEvent *event)
{
    if (acceptHoverEvents() && surface()) {
        WebOSCompositorWindow *w = static_cast<WebOSCompositorWindow *>(window());
#ifdef MULTIINPUT_SUPPORT
        QWaylandInputDevice *inputDevice = m_compositor->inputDeviceFor(event);
#else
        QWaylandInputDevice *inputDevice = w->inputDevice();
#endif
        if (inputDevice) {
            inputDevice->handle()->setMouseFocus(this, event->pos(), mapToScene(event->pos()));
            // In accessibility mode, there should be an explicit mouse move event
            // for an item where a hover event arrives.
            if (w->accessible())
                inputDevice->sendMouseMoveEvent(event->pos(), mapToScene(event->pos()));
        } else {
            qWarning() << "no input device for this event";
        }
    }
    m_compositor->notifyPointerEnteredSurface(this->surface());
}

void WebOSSurfaceItem::hoverLeaveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);

    if (acceptHoverEvents() && surface()) {
#ifdef MULTIINPUT_SUPPORT
        m_compositor->resetMouseFocus(surface());
#else
        QWaylandInputDevice *inputDevice = static_cast<WebOSCompositorWindow *>(window())->inputDevice();
        if (inputDevice)
            inputDevice->handle()->setMouseFocus(NULL, event->pos(), mapToScene(event->pos()));
        else
            qWarning() << "no input device for this event";
#endif
    }
    m_compositor->notifyPointerLeavedSurface(this->surface());
}

QWaylandInputDevice* WebOSSurfaceItem::getInputDevice(QInputEvent *event) const
{
#ifdef MULTIINPUT_SUPPORT
    if (!event)
        return nullptr;

    return m_compositor->inputDeviceFor(event);
#else
    if (!event || event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
        return m_compositor->keyboardDeviceForWindow(window());

    return m_compositor->defaultInputDevice();
#endif
}

void WebOSSurfaceItem::takeFocus(QWaylandInputDevice *device)
{
    QWaylandSurfaceItem::takeFocus(device ? device : getInputDevice());
}

void WebOSSurfaceItem::mouseUngrabEvent()
{
#ifdef MULTIINPUT_SUPPORT
    if (surface()) {
        QTouchEvent e(QEvent::TouchCancel);
        for (int i = 0; i < m_compositor->inputDevices().size()-1; i++) {
            QWaylandInputDevice *dev = m_compositor->inputDevices().at(i);
            if (!surface()->views().isEmpty() && dev && dev->mouseFocus() == surface()->views().first()) {
                e = QTouchEvent(QEvent::TouchCancel, Q_NULLPTR, (Qt::KeyboardModifiers)(static_cast<WebOSInputDevice*>(dev)->id()));
                break;
            }
        }
        QWaylandSurfaceItem::touchEvent(&e);
    }
#else
    QWaylandSurfaceItem::mouseUngrabEvent();
#endif
}

void WebOSSurfaceItem::processKeyEvent(QKeyEvent *event)
{
    if (!surface())
        return;

    QWaylandInputDevice *inputDevice = getInputDevice(event);
    if (!inputDevice) {
        qWarning() << "no input device for this event";
        return;
    }

    QtWayland::Keyboard *keyboard = inputDevice->handle()->keyboardDevice();

    // General case
    if (!(isPartOfGroup() || isSurfaceGroupRoot()) ||
        // If keyboard is grabbed, do not propagate key events between
        // window-group to avoid unintended keyboard focus change.
        keyboard->currentGrab() != keyboard) {

        if (hasFocus())
            inputDevice->sendFullKeyEvent(event);
        return;
    }

    // Surface group
    if (acceptsKeyEvent(event)) {
        inputDevice->setKeyboardFocus(surface());
        inputDevice->sendFullKeyEvent(event);
    } else if (m_surfaceGroup) {
        WebOSSurfaceItem *nextItem = NULL;
        if (m_surfaceGroup->allowLayerKeyOrder())
            nextItem = m_surfaceGroup->nextKeyOrderedSurfaceGroupItem(this);
        else
            nextItem = m_surfaceGroup->nextZOrderedSurfaceGroupItem(this);
        if (nextItem) {
            nextItem->processKeyEvent(event);
        }
    }
}

void WebOSSurfaceItem::keyPressEvent(QKeyEvent *event)
{
    PMTRACE_FUNCTION;
    processKeyEvent(event);
}

void WebOSSurfaceItem::keyReleaseEvent(QKeyEvent *event)
{
    PMTRACE_FUNCTION;
    processKeyEvent(event);
}

void WebOSSurfaceItem::focusInEvent(QFocusEvent *event)
{
    takeWlKeyboardFocus();
    QQuickItem::focusInEvent(event);
}

void WebOSSurfaceItem::focusOutEvent(QFocusEvent *event)
{
#ifdef MULTIINPUT_SUPPORT
    //Reset Keybaord/Pointer focus
    foreach (QWaylandInputDevice *dev, m_compositor->inputDevices()) {
        if (dev) {
            if (dev->keyboardFocus() == surface())
                dev->setKeyboardFocus(0);
            if (surface() && !surface()->views().isEmpty()
                && dev->mouseFocus() == surface()->views().first())
                dev->setMouseFocus(0, QPointF(), QPointF());
        }
    }
#else
    QWaylandInputDevice *keyboardDevice = getInputDevice();
    QWaylandInputDevice *mouseDevice = m_compositor->defaultInputDevice();
    if (keyboardDevice && keyboardDevice->keyboardFocus() == surface())
        keyboardDevice->setKeyboardFocus(0);
    if (mouseDevice && mouseDevice->mouseFocus() == this)
        mouseDevice->setMouseFocus(0, QPointF(), QPointF());
#endif

    m_compositor->setMouseEventEnabled(true);

    QQuickItem::focusOutEvent(event);
}

QVariantMap WebOSSurfaceItem::windowProperties()
{
    if (m_shellSurface) {
        return m_shellSurface->properties();
    }
    qWarning() << this << "m_shellSurface not available, use surface() instead";
    if (!isSurfaced()) {
        qWarning("null surface(), returning empty property map");
        return QVariantMap();
    }
    return surface()->windowProperties();
}

void WebOSSurfaceItem::setWindowProperty(const QString& key, const QVariant& value)
{
    if (m_shellSurface) {
        m_shellSurface->setProperty(key, value, false);
    } else {
        qWarning() << this << "m_shellSurface not available, use surface() instead";
        if (!isSurfaced()) {
            qWarning("null surface(), setting property is NOOP");
            return;
        }
        surface()->setWindowProperty(key, value);
    }
}

void WebOSSurfaceItem::updateProperties(const QVariantMap &properties, const QString &name, const QVariant &value)
{
    if (!surface()) {
        qWarning() << "ignoring property for an unsurfaced item" << this << name;
        return;
    }

    if (name == QLatin1String("appId")) {
        setAppId(value.toString());
    } else if (name == QLatin1String("_WEBOS_WINDOW_TYPE")) {
        setType(value.toString());
    } else if (name == QLatin1String("_WEBOS_WINDOW_CLASS")) {
        setWindowClass(WebOSSurfaceItem::WindowClass(value.toInt()), false);
    } else if (name == QLatin1String("title")) {
        setTitle(value.toString(), false);
    } else if (name == QLatin1String("subtitle")) {
        setSubtitle(value.toString(), false);
    } else if (name == QLatin1String("params")) {
        setParams(value.toString());
    } else if (name == QLatin1String("_WEBOS_LAUNCH_PREV_APP_AFTER_CLOSING")) {
        setLaunchLastApp(value.toBool());
    } else if (name == QLatin1String("displayAffinity")) {
        setDisplayAffinity(value.toInt());
    }

    emit windowPropertiesChanged(properties);
}

void WebOSSurfaceItem::setAppId(const QString& appId, bool updateProperty)
{
    PMTRACE_FUNCTION;
    if (m_appId != appId) {
        m_appId = appId;
        setObjectName(QString("surfaceItem_%1%2").arg(m_appId).arg(type()));
        emit appIdChanged();
        if (updateProperty)
            setWindowProperty(QLatin1String("appId"), m_appId);
    }
}

void WebOSSurfaceItem::setType(const QString& type, bool updateProperty)
{
    PMTRACE_FUNCTION;
    if (m_type != type) {
        m_type = type;
        setObjectName(QString("surfaceItem_%1%2").arg(appId()).arg(m_type));
        emit typeChanged();
        if (updateProperty)
            setWindowProperty(QLatin1String("_WEBOS_WINDOW_TYPE"), m_type);
    }
}

void WebOSSurfaceItem::setWindowClass(WebOSSurfaceItem::WindowClass wClass, bool updateProperty)
{
    PMTRACE_FUNCTION;
    if (m_windowClass != wClass) {
        m_windowClass = wClass;
        emit windowClassChanged();
        if (updateProperty)
            setWindowProperty(QLatin1String("_WEBOS_WINDOW_CLASS"), QVariant(m_windowClass));
    }
}

void WebOSSurfaceItem::setTitle(const QString& title, bool updateProperty)
{
    PMTRACE_FUNCTION;
    if (m_title != title) {
        m_title = title;
        emit titleChanged();
        if (updateProperty)
            setWindowProperty(QLatin1String("title"), m_title);
    }
}

void WebOSSurfaceItem::setSubtitle(const QString& subtitle, bool updateProperty)
{
    PMTRACE_FUNCTION;
    if (m_subtitle != subtitle) {
        m_subtitle = subtitle;
        emit subtitleChanged();
        if (updateProperty)
            setWindowProperty(QLatin1String("subtitle"), m_subtitle);
    }
}

void WebOSSurfaceItem::setParams(const QString& params, bool updateProperty)
{
    PMTRACE_FUNCTION;
    if (m_params != params) {
        m_params = params;
        emit paramsChanged();
        if (updateProperty)
            setWindowProperty(QLatin1String("params"), m_params);
    }
}

void WebOSSurfaceItem::setLaunchLastApp(const bool& launchLastApp, bool updateProperty)
{
    PMTRACE_FUNCTION;
    if (m_launchLastApp != launchLastApp) {
        m_launchLastApp = launchLastApp;
        emit launchLastAppChanged();
        if (updateProperty)
            setWindowProperty(QLatin1String("_WEBOS_LAUNCH_PREV_APP_AFTER_CLOSING"), m_launchLastApp);
    }
}

void WebOSSurfaceItem::setTransientModel(WebOSWindowModel* model)
{
    PMTRACE_FUNCTION;
    if (model != m_transientModel) {
        m_transientModel = model;
        m_transientModel->setSurfaceSource(m_compositor->surfaceModel());
        emit transientModelChanged();
    }
}

void WebOSSurfaceItem::setGroupedWindowModel(WebOSGroupedWindowModel* model)
{
    PMTRACE_FUNCTION;
    if (model != m_groupedWindowModel) {
        m_groupedWindowModel = model;
        emit groupedWindowModelChanged();
    }
}

bool WebOSSurfaceItem::isTransient() const
{
    if( !isSurfaced() )
        return false;

    return surface()->transientParent() != NULL;
}

WebOSSurfaceItem* WebOSSurfaceItem::transientParent()
{
    if( !isSurfaced() )
        return NULL;

    QWaylandQuickSurface* parent = qobject_cast<QWaylandQuickSurface *>(surface()->transientParent());
    if (parent) {
        return static_cast<WebOSSurfaceItem*>(parent->surfaceItem());
    }
    return NULL;
}

void WebOSSurfaceItem::setCardSnapShotFilePath(const QString& fPath)
{
    if (m_cardSnapShotFilePath != fPath) {
        m_cardSnapShotFilePath = fPath;
        emit cardSnapShotFilePathChanged();
    }
}

void WebOSSurfaceItem::setCustomImageFilePath(QString filePath)
{
    if (m_customImageFilePath != filePath) {
        m_customImageFilePath = filePath;
        emit customImageFilePathChanged();
    }
}

void WebOSSurfaceItem::setBackgroundImageFilePath(QString filePath)
{
    QString path = filePath;
    QString filePrefix = QLatin1String("file://");
    QString pvrPrefix = QLatin1String("image://compressed");

    if (path.startsWith(filePrefix)) {
        path.remove(0, filePrefix.length());
    } else if (path.startsWith(pvrPrefix)) {
        path.remove(0, pvrPrefix.length());
    }
    if (m_backgroundImageFilePath != path) {
        m_backgroundImageFilePath = path;
        emit backgroundImageFilePathChanged();
    }
}

void WebOSSurfaceItem::deleteSnapShot()
{
    QString filepath = getCardSnapShotFilePath();
    if (filepath == customImageFilePath() ||
        filepath == backgroundImageFilePath()) {
        return;
    }

    QFile oldFile(filepath);
    if (oldFile.exists()) {
        oldFile.remove();
    }
}

void WebOSSurfaceItem::prepareState(Qt::WindowState s)
{
    if (m_shellSurface)
        m_shellSurface->prepareState(s);
}

void WebOSSurfaceItem::setState(Qt::WindowState s)
{
    PMTRACE_FUNCTION;
    if (m_shellSurface) {
        m_shellSurface->setState(s);
        //If state is minimized, ignore all input events.
        setEnabled(Qt::WindowMinimized != m_shellSurface->state());
    } else {
        qWarning() << "No webos shell surface exist, cannot set state" << s;
    }
    emit stateChanged();
}

Qt::WindowState WebOSSurfaceItem::state()
{
    return m_shellSurface ? m_shellSurface->state() : Qt::WindowFullScreen;
}

void WebOSSurfaceItem::close()
{
    PMTRACE_FUNCTION;
    if (m_shellSurface) {
        sendCloseToGroupItems();
        m_shellSurface->close();
    } else {
        qWarning() << "No webos shell surface exist, cannot close";
    }
}

void WebOSSurfaceItem::sendCloseToGroupItems()
{
    PMTRACE_FUNCTION;
    if (isSurfaceGroupRoot()) {
        m_surfaceGroup->closeAttachedSurfaces();
    }
}

void WebOSSurfaceItem::setShellSurface(WebOSShellSurface* shell)
{
    PMTRACE_FUNCTION;
    if (shell && m_shellSurface != shell) {
        delete m_shellSurface;
        m_shellSurface = shell;
        connect(m_shellSurface, SIGNAL(locationHintChanged()), this, SIGNAL(locationHintChanged()));
        connect(m_shellSurface, SIGNAL(keyMaskChanged()), this, SIGNAL(keyMaskChanged()));
        connect(m_shellSurface, SIGNAL(stateChangeRequested(Qt::WindowState)), this, SLOT(requestStateChange(Qt::WindowState)));
        connect(m_shellSurface, SIGNAL(propertiesChanged(QVariantMap, QString, QVariant)),
                this, SLOT(updateProperties(QVariantMap, QString, QVariant)));
    }
}

WebOSSurfaceItem::LocationHints WebOSSurfaceItem::locationHint()
{
    return m_shellSurface ? m_shellSurface->locationHint() : LocationHintNorth  | LocationHintEast;
}

WebOSSurfaceItem::KeyMasks WebOSSurfaceItem::keyMask() const
{
    return m_shellSurface ? m_shellSurface->keyMask() : KeyMaskDefault;
}

void WebOSSurfaceItem::setItemState(ItemState state, const QString &reason)
{
    setItemStateReason(reason);

    if (m_itemState != state) {
        m_itemState = state;
        emit itemStateChanged();
        emit dataChanged();
    }
}

void WebOSSurfaceItem::setItemStateReason(const QString &reason)
{
    if (m_itemStateReason != reason) {
        m_itemStateReason = reason;

        emit itemStateReasonChanged();
    }
}

void WebOSSurfaceItem::setClosePolicy(QVariantMap &policy)
{
    if (m_closePolicy != policy) {
        m_closePolicy = policy;

        emit closePolicyChanged();
    }
}

void WebOSSurfaceItem::updateScreenPosition()
{
    if (m_shellSurface && m_notifyPositionToClient) {
        m_shellSurface->setPosition(mapToScene(QPointF(0, 0)));
    }
}

void WebOSSurfaceItem::requestStateChange(Qt::WindowState state)
{
    switch (state) {
        case Qt::WindowNoState:
        case Qt::WindowMaximized:
            qWarning() << "not yet supported to change state to" << state;
            break;
        case Qt::WindowMinimized:
            requestMinimize();
            break;
        case Qt::WindowFullScreen:
            requestFullscreen();
            break;
        default:
           qWarning() << "unknown state!";
           break;
    }
}

void WebOSSurfaceItem::onSurfaceDamaged(const QRegion &region)
{
    PMTRACE_FUNCTION;
    Q_UNUSED(region);
    PMTRACE_KEY_VALUE_LOG("appFirstFrame", (char *)appId().toStdString().c_str());

    /* Some surfaces can try to render after it is detached from scengraph.
       In that case, if compositor allows the rendering, compositor needs
       to call frameFinished. It will release previous front buffer
       of the surface. Otherwise, the surface will be stuck to wait for
       available buffer. */
    if (!window() && surface())
       m_compositor->sendFrameCallbacks(QList<QWaylandSurface *>() << surface());
}

void WebOSSurfaceItem::resizeClientTo(int width, int height)
{
    if (!isSurfaced()) {
        qWarning("failed, null surface()");
        return;
    }
    return surface()->requestSize(QSize(width, height));
}

void WebOSSurfaceItem::setNotifyPositionToClient(bool notify)
{

    if (m_notifyPositionToClient != notify) {
        m_notifyPositionToClient = notify;
        emit notifyPositionToClientChanged();
        if (m_notifyPositionToClient) {
            updateScreenPosition();
        }
    }
}

void WebOSSurfaceItem::setExposed(bool exposed)
{
    if (m_exposed != exposed) {
        if (m_shellSurface && surface()) {
            QRegion r = exposed ? QRegion(QRect(QPoint(0, 0), surface()->size())) : QRegion();
            m_shellSurface->exposed(r);
        } else {
            qWarning("no surface or shellSurface, no one to send to.");
        }
        m_exposed = exposed;
        emit exposedChanged();
    }
}

void WebOSSurfaceItem::setLaunchRequired(bool required)
{
    if (m_launchRequired != required) {
        m_launchRequired = required;
        emit launchRequiredChanged();
    }
}

void WebOSSurfaceItem::setSurfaceGroup(WebOSSurfaceGroup* group)
{
    PMTRACE_FUNCTION;
    if (group != m_surfaceGroup) {

        m_surfaceGroup = group;

        if (!m_surfaceGroup) {
            if (m_groupedWindowModel) {
                emit m_groupedWindowModel->surfaceRemoved(this);
            }
        }

        emit surfaceGroupChanged();
        emit dataChanged();
    }
}

bool WebOSSurfaceItem::isPartOfGroup()
{
    // The root item of a surface is not considered to be part of a group
    return m_surfaceGroup && m_surfaceGroup->isValid() && m_surfaceGroup->rootItem() != this;
}

bool WebOSSurfaceItem::isSurfaceGroupRoot()
{
    return m_surfaceGroup && m_surfaceGroup->rootItem() == this;
}

bool WebOSSurfaceItem::acceptsKeyEvent(QKeyEvent *event) const
{
    return keyMask() & keyMaskFromQt(event->key());
}

WebOSSurfaceItem::KeyMasks WebOSSurfaceItem::keyMaskFromQt(int key) const
{
    KeyMasks retKeyMask = KeyMaskDefault;

    switch(key) {
    case Qt::Key_Super_L:
        retKeyMask = KeyMaskHome;
    break;
    case Qt::Key_webOS_Back:
        retKeyMask = KeyMaskBack;
    break;
    case Qt::Key_webOS_Exit:
        retKeyMask = KeyMaskExit;
    break;
    case Qt::Key_Left:
        retKeyMask = KeyMaskLeft;
    break;
    case Qt::Key_webOS_LocalLeft:
        retKeyMask = KeyMaskLocalLeft;
    break;
    case Qt::Key_Up:
        retKeyMask = KeyMaskUp;
    break;
    case Qt::Key_webOS_LocalUp:
        retKeyMask = KeyMaskLocalUp;
    break;
    case Qt::Key_PageUp:
        retKeyMask = KeyMaskUp;
    break;
    case Qt::Key_Right:
        retKeyMask = KeyMaskRight;
    break;
    case Qt::Key_webOS_LocalRight:
        retKeyMask = KeyMaskLocalRight;
    break;
    case Qt::Key_Down:
        retKeyMask = KeyMaskDown;
    break;
    case Qt::Key_webOS_LocalDown:
        retKeyMask = KeyMaskLocalDown;
    break;
    case Qt::Key_PageDown:
        retKeyMask = KeyMaskDown;
    break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        retKeyMask = KeyMaskOk;
    break;
    case Qt::Key_webOS_LocalEnter:
        retKeyMask = KeyMaskLocalOk;
    break;
    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
        retKeyMask = KeyMaskNumeric;
    break;
    case Qt::Key_Red:
        retKeyMask = KeyMaskRemoteColorRed;
    break;
    case Qt::Key_webOS_Red:
        retKeyMask = KeyMaskRemoteColorRed;
    break;
    case Qt::Key_Green:
        retKeyMask = KeyMaskRemoteColorGreen;
    break;
    case Qt::Key_webOS_Green:
        retKeyMask = KeyMaskRemoteColorGreen;
    break;
    case Qt::Key_Yellow:
        retKeyMask = KeyMaskRemoteColorYellow;
    break;
    case Qt::Key_webOS_Yellow:
        retKeyMask = KeyMaskRemoteColorYellow;
    break;
    case Qt::Key_Blue:
        retKeyMask = KeyMaskRemoteColorBlue;
    break;
    case Qt::Key_webOS_Blue:
        retKeyMask = KeyMaskRemoteColorBlue;
    break;
    case Qt::Key_webOS_ChannelUp:
    case Qt::Key_webOS_ChannelDown:
    case Qt::Key_webOS_ChannelDash:
    case Qt::Key_webOS_ChannelBack:
        retKeyMask = KeyMaskRemoteProgrammeGroup;
    break;
    case Qt::Key_MediaPlay:
    case Qt::Key_MediaStop:
    case Qt::Key_MediaPause:
    case Qt::Key_AudioRewind:
    case Qt::Key_AudioForward:
        retKeyMask = KeyMaskRemotePlaybackGroup | KeyMaskMinimalPlaybackGroup;
    break;
    case Qt::Key_MediaPrevious:
    case Qt::Key_MediaNext:
    case Qt::Key_MediaRecord:
    case Qt::Key_MediaTogglePlayPause:
        retKeyMask = KeyMaskRemotePlaybackGroup;
    break;
    case Qt::Key_BackForward:
        retKeyMask = KeyMaskRemotePlaybackGroup;
    break;
    case Qt::Key_AudioRepeat:
    case Qt::Key_AudioRandomPlay:
        retKeyMask = KeyMaskRemotePlaybackGroup;
    break;
    case Qt::Key_AudioCycleTrack:
        retKeyMask = KeyMaskRemotePlaybackGroup;
    break;
    case Qt::Key_webOS_Teletext:
        retKeyMask = KeyMaskRemoteTeletextGroup;
    break;
    case Qt::Key_webOS_ProgramList:
    case Qt::Key_webOS_Info:
    case Qt::Key_webOS_MagnifierZoom:
    case Qt::Key_webOS_LiveZoom:
        retKeyMask = KeyMaskRemoteMagnifierGroup;
    break;
    case Qt::Key_webOS_TVGuide:
        retKeyMask = KeyMaskGuide;
    break;
    case Qt::Key_webOS_TeletextSize:
    case Qt::Key_webOS_TextOption:
    case Qt::Key_webOS_TextMode:
    case Qt::Key_webOS_TextMix:
    case Qt::Key_webOS_TeletextReveal:
    case Qt::Key_webOS_TeletextInTime:
    case Qt::Key_webOS_TeletextHold:
    case Qt::Key_webOS_TeletextPosition:
    case Qt::Key_webOS_TeletextSubPage:
    case Qt::Key_webOS_TeletextFreeze:
    case Qt::Key_webOS_MultiPip:
        retKeyMask = KeyMaskTeletextActiveGroup;
    break;
    default:
        /* If a client want to receive some special keys and the other keys have to be delivered to the parent,
             there are two approaches according to kinds of special keys.
             1) Set the key mask value with bitwise OR operation against the supported individual key masks
             2) Set the key mask value with bitwise XOR operation against the default key mask value and supported
             individual key masks.
             For above two approaches, the acceptsKeyEvent method use the bitwise AND operation against the keyMask
             of the client and retKeyMask value.
             And the retKeyMask value of default case have to be applied bitwise XOR operation against default key mask value
             and supported individual key masks for the keys without supported individual key mask. */
        retKeyMask = (KeyMasks) KeyMaskDefault
                     ^ KeyMaskLeft
                     ^ KeyMaskRight
                     ^ KeyMaskUp
                     ^ KeyMaskDown
                     ^ KeyMaskOk
                     ^ KeyMaskNumeric
                     ^ KeyMaskRemoteColorRed
                     ^ KeyMaskRemoteColorGreen
                     ^ KeyMaskRemoteColorYellow
                     ^ KeyMaskRemoteColorBlue
                     ^ KeyMaskRemoteProgrammeGroup
                     ^ KeyMaskRemotePlaybackGroup
                     ^ KeyMaskRemoteTeletextGroup
                     ^ KeyMaskLocalLeft
                     ^ KeyMaskLocalRight
                     ^ KeyMaskLocalUp
                     ^ KeyMaskLocalDown
                     ^ KeyMaskLocalOk
                     ^ KeyMaskRemoteMagnifierGroup
                     ^ KeyMaskMinimalPlaybackGroup
                     ^ KeyMaskGuide
                     ^ KeyMaskTeletextActiveGroup;
    break;
    }

    return retKeyMask;
}

/* This BufferAttacher is from qwindow compositor example in QtWayland */
class BufferAttacher : public QWaylandBufferAttacher
{
public:
    BufferAttacher()
        : QWaylandBufferAttacher()
          , shmTex(0)
          , bufferRef(QWaylandBufferRef())
    {
    }

    ~BufferAttacher()
    {
        delete shmTex;
    }

    BufferAttacher(const BufferAttacher&) = delete;
    BufferAttacher &operator=(const BufferAttacher&) = delete;

    void attach(const QWaylandBufferRef &ref) Q_DECL_OVERRIDE
    {
        if (bufferRef) {
            if (bufferRef.isShm()) {
                delete shmTex;
                shmTex = 0;
            } else {
                bufferRef.destroyTexture();
            }
        }

        bufferRef = ref;

        if (bufferRef) {
            if (bufferRef.isShm()) {
                shmTex = new QOpenGLTexture(bufferRef.image(), QOpenGLTexture::DontGenerateMipMaps);
                texture = shmTex->textureId();
            } else {
                texture = bufferRef.createTexture();
            }
        }
    }

    void unmap()
    {
        if (bufferRef) {
            if (bufferRef.isShm()) {
                delete shmTex;
                shmTex = NULL;
            } else {
                bufferRef.destroyTexture();
            }
        }
        bufferRef = QWaylandBufferRef();
    }

    QImage image() const
    {
        if (!bufferRef || !bufferRef.isShm())
            return QImage();
        return bufferRef.image();
    }

    QOpenGLTexture *shmTex;
    QWaylandBufferRef bufferRef;
    GLuint texture;
};

bool WebOSSurfaceItem::getCursorFromSurface(QWaylandSurface *surface, int hotSpotX, int hotSpotY, QCursor& cursor)
{
    if (surface && surface->bufferAttacher()) {
        QImage image = static_cast<BufferAttacher *>(surface->bufferAttacher())->image();
        if (!image.isNull()) {
            if (hotSpotX >= 0 && hotSpotX <= image.size().width() &&
                hotSpotY >= 0 && hotSpotY <= image.size().height()) {
                cursor = QCursor(QPixmap::fromImage(image), hotSpotX, hotSpotY);
                if (cursor.shape() == Qt::BitmapCursor) {
                    return true;
                } else {
                    qWarning() << "Cursor: unable to convert surface into a bitmap cursor";
                    return false;
                }
            } else {
                qWarning() << "Cursor: hotspot" << hotSpotX <<  hotSpotY << "is out of" << image.size();
                return false;
            }
        }
    }

    qWarning() << "Cursor: surface has no content";

    return false;
}

void WebOSSurfaceItem::setCursorSurface(QWaylandSurface *surface, int hotSpotX, int hotSpotY)
{
    if (m_cursorSurface == surface && m_cursorHotSpotX == hotSpotX && m_cursorHotSpotY == hotSpotY) {
        qWarning() << "Cursor: attempting to set the same cursor surface, ignored" << surface << hotSpotX << hotSpotY;
    } else {
        qDebug() << "Cursor: updating cursor with surface" << surface << hotSpotX << hotSpotY;

        QCursor cursor;
        bool staticCursor = false;

        if (m_compositor->getCursor(surface, hotSpotX, hotSpotY, cursor)) {
            qDebug() << "Cursor: use the cursor designated by compositor" << cursor;
            staticCursor = true;
        } else if (!surface) {
            cursor = QCursor(Qt::BlankCursor);
            staticCursor = true;
        } else {
            staticCursor = false;
        }

        if (m_cursorSurface) {
            qDebug() << "Cursor: disconnect old cursor surface" << m_cursorSurface.data() << "static:" << staticCursor;
            QObject::disconnect(m_cursorSurface.data(), SIGNAL(redraw()), this, SLOT(updateCursor()));
            m_cursorSurface->setBufferAttacher(0);
        }

        if (staticCursor) {
            qDebug() << "Cursor: set a static cursor" << cursor;
            setCursor(cursor);
        } else if (surface) {
            qDebug() << "Cursor: set a live cursor with cursor surface" << surface << "static:" << staticCursor;
            connect(surface, SIGNAL(redraw()), this, SLOT(updateCursor()), Qt::UniqueConnection);
            surface->setBufferAttacher(new BufferAttacher());
            if (getCursorFromSurface(surface, hotSpotX, hotSpotY, cursor)) {
                setCursor(cursor);
            } else {
                qWarning() << "Cursor: fallback to the default cursor";
                setCursor(QCursor(Qt::ArrowCursor));
            }
        }

        m_cursorSurface = surface;
        m_cursorHotSpotX = hotSpotX;
        m_cursorHotSpotY = hotSpotY;
    }
}

void WebOSSurfaceItem::updateCursor()
{
    QList<QWaylandSurface *> surfaces;

    if (m_cursorSurface) {
        QCursor c;
        qDebug() << "Cursor: live updating cursor with surface" << m_cursorSurface.data() << m_cursorHotSpotX << m_cursorHotSpotY;
        if (getCursorFromSurface(m_cursorSurface.data(), m_cursorHotSpotX, m_cursorHotSpotY, c))
            setCursor(c);
        surfaces << m_cursorSurface.data();
    }
    m_compositor->sendFrameCallbacks(surfaces);
}

WebOSSurfaceItem *WebOSSurfaceItem::createMirrorItem(int target)
{
    if (m_mirrorItems.contains(target))
        return nullptr;

    WebOSSurfaceItem *mirror = new WebOSSurfaceItem(m_compositor, static_cast<QWaylandQuickSurface *>(surface()));
    mirror->setDisplayAffinity(target);
    mirror->setEnabled(false);
    mirror->setAppId(appId());
    mirror->setType(type());
    mirror->setResizeSurfaceToItem(false);
    mirror->setItemState(WebOSSurfaceItem::ItemStateNormal);

    if (exported())
        exported()->startImportedMirroring(mirror);

    m_mirrorItems[target] = mirror;
    return mirror;
}
