// Copyright (c) 2013-2021 LG Electronics, Inc.
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

#include "webostablet/webostablet.h"

#include <QDateTime>
#include <QQmlEngine>
#include <QOpenGLTexture>
#include <QDebug>

#include <qweboskeyextension.h>

#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qsgrenderer_p.h>

#include <QtWaylandCompositor/qwaylandseat.h>
#include <QtWaylandCompositor/private/qwaylandkeyboard_p.h>
#include <QtWaylandCompositor/private/qwaylandpointer_p.h>
#include <QtWaylandCompositor/private/qwaylandsurface_p.h>
#include <QtWaylandCompositor/qwaylandbufferref.h>
#include <QtWaylandCompositor/private/qwaylandquickhardwarelayer_p.h>

#include "weboscompositortracer.h"
#include "weboskeyboard.h"
#include "webossurface.h"

WebOSSurfaceItem::WebOSSurfaceItem(WebOSCoreCompositor* compositor, QWaylandQuickSurface* quickSurface)
        : QWaylandQuickItem()
        , m_compositor(compositor)
        , m_fullscreen(false)
        , m_lastFullscreenTick(0)
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
        , m_userId()
        , m_exposed(false)
        , m_launchRequired(false)
        , m_surfaceGroup(0)
        , m_hasKeyboardFocus(false)
        , m_grabKeyboardFocusOnClick(true)
        , m_closePolicy(QVariantMap())
        , m_itemStateReason(QString())
        , m_launchLastApp(false)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    setAcceptTouchEvents(true);
#endif
    WebOSSurface *surface = static_cast<WebOSSurface*>(quickSurface);
    setSurface(surface);

    if (surface) {
        connect(surface, SIGNAL(damaged(const QRegion &)), this, SLOT(onSurfaceDamaged(const QRegion &)));
        connect(surface, &WebOSSurface::aboutToBeDestroyed, this, &WebOSSurfaceItem::itemAboutToBeHidden);
        connect(surface, &WebOSSurface::nullBufferAttached, this, &WebOSSurfaceItem::itemAboutToBeHidden);
    }

    connect(this, &QQuickItem::xChanged, this, &WebOSSurfaceItem::updateScreenPosition);
    connect(this, &QQuickItem::yChanged, this, &WebOSSurfaceItem::updateScreenPosition);
    // Send position_changed whenever the surface resizes
    // so that the client gets notified that the resize is done
    connect(this, &QQuickItem::widthChanged, this, &WebOSSurfaceItem::updateScreenPosition);
    connect(this, &QQuickItem::heightChanged, this, &WebOSSurfaceItem::updateScreenPosition);
    // Set the ownership as CppOwnership explicitly to prevent from garbage collecting by JS engine
    QQmlEngine::setObjectOwnership((QObject*)this, QQmlEngine::CppOwnership);

    //In Qt5.2, All MouseAreas have default cursor as default value. So all surfaceitems should have default cursor as a initial value so that
    //it can restore the cursor from the system ui's cursor. See QQuickWindowPrivate::updateCursor()
    setCursor(Qt::ArrowCursor);

    connect(this, &QQuickItem::windowChanged, this, &WebOSSurfaceItem::handleWindowChanged);

    setObjectName(QStringLiteral("surfaceItem_default"));
    if (surface)
        surface->setObjectName(QStringLiteral("surface_default"));

    setSmooth(true);
}

WebOSSurfaceItem::~WebOSSurfaceItem()
{
    emit itemAboutToBeDestroyed();

    disconnect(this, &QQuickItem::windowChanged, this, &WebOSSurfaceItem::handleWindowChanged);

    sendCloseToGroupItems();
    if (isSurfaceGroupRoot())
        m_surfaceGroup->setRootItem(NULL);
    m_surfaceGroup = NULL;
    deleteSnapShot();
    delete m_shellSurface;

    if (m_mirrorItems.size() > 0)
        qCritical() << "Zombie mirror items, should not happen";

    if (m_surfaceGrabbed)
        qCritical() << "m_surfaceGrabbed should be null at this point";
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

    // Unset direct update whenever it gets removed from the scene
    if (!window())
        setDirectUpdateOnPlane(false);
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

QList<QTouchEvent::TouchPoint> WebOSSurfaceItem::mapToTarget(const QList<QTouchEvent::TouchPoint>& points) const
{
    QList<QTouchEvent::TouchPoint> result;
    foreach (QTouchEvent::TouchPoint point, points) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        auto &p = QMutableEventPoint::from(point);
        p.setPosition(mapToSurface(point.position()));
        result.append(p);
#else
        point.setPos(mapToSurface(point.pos()));
        result.append(point);
#endif
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
    foreach (QWaylandSeat *dev, m_compositor->inputDevices())
        if (dev)
            dev->setKeyboardFocus(surface());
#else
    /* set keyboard focus for this item */
    QWaylandSeat *dev = m_compositor->keyboardDeviceForWindow(window());
    if (dev)
        dev->setKeyboardFocus(surface());
#endif
}

bool WebOSSurfaceItem::contains(const QPointF& point) const
{
    return inputRegionContains(point);
}

bool WebOSSurfaceItem::isMapped()
{
    return m_compositor && m_compositor->isMapped(this);
}

bool WebOSSurfaceItem::event(QEvent* ev)
{
    switch (ev->type()) {
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
        return tabletEvent((QTabletEvent*)ev);
    default:
        break;
    }

    // TODO: Check the closest the super class that overrides event()
    return QQuickItem::event(ev);
}

bool WebOSSurfaceItem::tabletEvent(QTabletEvent* event)
{
    if (!surface()) {
        qWarning() << "Ignoring tabletEvent sent to surface item that has no surface" << this;
        return false;
    }

    if (m_compositor->tabletDevice())
        return m_compositor->tabletDevice()->postTabletEvent(event, surface()->primaryView());

    return false;
}

void WebOSSurfaceItem::hoverMoveEvent(QHoverEvent *event)
{
    if (acceptHoverEvents()) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        QMouseEvent e(QEvent::MouseMove, event->position(), Qt::NoButton, Qt::NoButton, event->modifiers());
#else
        QMouseEvent e(QEvent::MouseMove, event->pos(), Qt::NoButton, Qt::NoButton, event->modifiers());
#endif
        mouseMoveEvent(&e);
    }
}

void WebOSSurfaceItem::mouseMoveEvent(QMouseEvent * event)
{
    if (!window()) {
        qWarning() << "Ignoring mouseMoveEvent sent to surface item that has no window" << this;
        return;
    }

    if (!surface()) {
        qWarning() << "Ignoring mouseMoveEvent sent to surface item that has no surface" << this;
        return;
    }

    // Make sure client to receive the latest position before the mouse event
    updateScreenPosition();

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    WebOSMouseEvent e(event->type(), event->position(), event->button(), event->buttons(), event->modifiers(), window());
#else
    WebOSMouseEvent e(event->type(), event->pos(), event->button(), event->buttons(), event->modifiers(), window());
#endif
    QWaylandQuickItem::mouseMoveEvent(&e);
}

void WebOSSurfaceItem::mousePressEvent(QMouseEvent *event)
{
    if (!window()) {
        qWarning() << "Ignoring mousePressEvent sent to surface item that has no window" << this;
        return;
    }

    if (!surface()) {
        qWarning() << "Ignoring mousePressEvent sent to surface item that has no surface" << this;
        return;
    }

    WebOSMouseEvent e(event->type(), event->localPos().toPoint(),
                  event->button(), event->buttons(), event->modifiers(), window());

    WebOSCompositorWindow *w = static_cast<WebOSCompositorWindow *>(window());
#ifdef MULTIINPUT_SUPPORT
    QWaylandSeat *inputDevice = getInputDevice(&e);
    QWaylandSeat *keyboardDevice = inputDevice;
#else
    QWaylandSeat *inputDevice = w->inputDevice();
    QWaylandSeat *keyboardDevice = getInputDevice();
#endif
    if (inputDevice && keyboardDevice) {
        if (inputDevice->mouseFocus() != view())
            inputDevice->setMouseFocus(view());

        if (inputDevice->mouseFocus()
                && inputDevice->mouseFocus()->surface() != keyboardDevice->keyboardFocus()
                && m_grabKeyboardFocusOnClick) {
            takeWlKeyboardFocus();
            m_hasKeyboardFocus = true;
            emit hasKeyboardFocusChanged();
        }

        if (!w->accessible()) {
            // Send extra mouse move event as otherwise the client
            // will handle the button event in the incorrect coordinate
            // in case the surface size is changed.
            mouseMoveEvent(&e);
            inputDevice->sendMousePressEvent(e.button());
        } else {
            // In accessibility mode there should be no extra mouse move event sent.
            inputDevice->setMouseFocus(view());
            inputDevice->sendMousePressEvent(e.button());
        }
    } else {
        qWarning() << "no input device for this event";
    }
}

void WebOSSurfaceItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (!window()) {
        qWarning() << "Ignoring mouseReleaseEvent sent to surface item that has no window" << this;
        return;
    }

    if (!surface()) {
        qWarning() << "Ignoring mouseReleaseEvent sent to surface item that has no surface" << this;
        return;
    }

    WebOSMouseEvent e(event->type(), event->localPos().toPoint(),
                  event->button(), event->buttons(), event->modifiers(), window());

    WebOSCompositorWindow *w = static_cast<WebOSCompositorWindow *>(window());
#ifdef MULTIINPUT_SUPPORT
    QWaylandSeat *inputDevice = getInputDevice(&e);
#else
    QWaylandSeat *inputDevice = w->inputDevice();
#endif

    if (!w->accessible()) {
        // Send extra mouse move event as otherwise the client
        // will handle the button event in the incorrect coordinate
        // in case the surface size is changed.
        mouseMoveEvent(&e);
        QWaylandQuickItem::mouseReleaseEvent(&e);
    } else {
        // In accessibility mode there should be no extra mouse move event sent.
        if (inputDevice) {
            inputDevice->setMouseFocus(view());
            inputDevice->sendMouseReleaseEvent(e.button());
        } else {
            qWarning() << "no input device for this event";
        }
    }
}

void WebOSSurfaceItem::wheelEvent(QWheelEvent *event)
{
    if (!window()) {
        qWarning() << "Ignoring wheelEvent sent to surface item that has no window" << this;
        return;
    }

    if (!surface()) {
        qWarning() << "Ignoring wheelEvent sent to surface item that has no surface" << this;
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    WebOSWheelEvent e(event->position(), event->globalPosition(), event->pixelDelta(), event->angleDelta(),
            event->buttons(), event->modifiers(), event->phase(), window(),
            event->inverted());
#else
    WebOSWheelEvent e(event->pos(), event->globalPos(), event->pixelDelta(), event->angleDelta(),
            event->delta(), event->orientation(),
            event->buttons(), event->modifiers(), event->phase(), window());
#endif

#ifdef MULTIINPUT_SUPPORT
    QWaylandSeat *inputDevice = getInputDevice(&e);
#else
    QWaylandSeat *inputDevice = static_cast<WebOSCompositorWindow *>(window())->inputDevice();
#endif
    if (inputDevice && inputDevice->mouseFocus() != view())
        inputDevice->setMouseFocus(view());

    // Send extra mouse move event as otherwise the client
    // will handle the wheel event in the incorrect coordinate
    // in case the surface size is changed.
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QMouseEvent ee(QEvent::MouseMove, event->position(), Qt::NoButton, Qt::NoButton, event->modifiers());
#else
    QMouseEvent ee(QEvent::MouseMove, event->pos(), Qt::NoButton, Qt::NoButton, event->modifiers());
#endif
    mouseMoveEvent(&ee);

    QWaylandQuickItem::wheelEvent(&e);
}

void WebOSSurfaceItem::touchEvent(QTouchEvent *event)
{
    if (!window()) {
        qWarning() << "Ignoring touchEvent sent to surface item that has no window" << this;
        return;
    }

    if (!surface()) {
        qWarning() << "Ignoring touchEvent sent to surface item that has no surface" << this;
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QTouchEvent e(event->type(), event->pointingDevice(), event->modifiers(), event->touchPointStates(), mapToTarget(event->touchPoints()));
#else
    QTouchEvent e(event->type(), event->device(), event->modifiers(), event->touchPointStates(), mapToTarget(event->touchPoints()));
    e.setWindow(event->window());
#endif

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    // This may not be needed with QtWayland 5.12
    // Currently, this is due to QWaylandSurfaceItem::mouseUngrabEvent
    // which sends the Cancel without window().
    if (!event->window())
        e.setWindow(window());
#endif

    if (inputEventsEnabled() && touchEventsEnabled()) {
        WebOSCompositorWindow *w = static_cast<WebOSCompositorWindow *>(window());

#ifdef MULTIINPUT_SUPPORT
        QWaylandSeat *seat = getInputDevice(&e);
#else
        QWaylandSeat *seat = w->inputDevice();
#endif

        QPoint pointPos;
        const QList<QTouchEvent::TouchPoint> &points = e.touchPoints();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        if (!points.isEmpty())
            pointPos = points.at(0).position().toPoint();
#else
        if (!points.isEmpty())
            pointPos = points.at(0).pos().toPoint();
#endif

        if (e.type() == QEvent::TouchBegin && !surface()->inputRegionContains(pointPos))
            return;

        if (seat->mouseFocus() != view())
            seat->sendMouseMoveEvent(view(), pointPos, mapToScene(pointPos));
        seat->sendFullTouchEvent(surface(), &e);
    } else {
        QWaylandQuickItem::touchEvent(event);
    }

    event->setAccepted(e.isAccepted());
}

void WebOSSurfaceItem::hoverEnterEvent(QHoverEvent *event)
{
    if (!window()) {
        qWarning() << "Ignoring hoverEnterEvent sent to surface item that has no window" << this;
        return;
    }

    if (!surface()) {
        qWarning() << "Ignoring hoverEnterEvent sent to surface item that has no surface" << this;
        return;
    }

    if (acceptHoverEvents()) {
        WebOSCompositorWindow *w = static_cast<WebOSCompositorWindow *>(window());
#ifdef MULTIINPUT_SUPPORT
        QWaylandSeat *inputDevice = m_compositor->seatFor(event);
#else
        QWaylandSeat *inputDevice = w->inputDevice();
#endif
        if (inputDevice) {
            inputDevice->setMouseFocus(view());
            // In accessibility mode, there should be an explicit mouse move event
            // for an item where a hover event arrives.
            if (w->accessible())
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
                inputDevice->sendMouseMoveEvent(view(), event->position(), mapToScene(event->position()));
#else
                inputDevice->sendMouseMoveEvent(view(), event->pos(), mapToScene(event->pos()));
#endif
        } else {
            qWarning() << "no input device for this event";
        }
    }
    m_compositor->notifyPointerEnteredSurface(surface());
    QWaylandQuickItem::hoverEnterEvent(event);
}

void WebOSSurfaceItem::hoverLeaveEvent(QHoverEvent *event)
{
    if (!window()) {
        qWarning() << "Ignoring hoverLeaveEvent sent to surface item that has no window" << this;
        return;
    }

    if (!surface()) {
        qWarning() << "Ignoring hoverLeaveEvent sent to surface item that has no surface" << this;
        return;
    }

    if (acceptHoverEvents()) {
#ifdef MULTIINPUT_SUPPORT
        m_compositor->resetMouseFocus(surface());
#else
        QWaylandSeat *inputDevice = static_cast<WebOSCompositorWindow *>(window())->inputDevice();
        if (inputDevice)
            inputDevice->setMouseFocus(nullptr);
        else
            qWarning() << "no input device for this event";
#endif
    }
    m_compositor->notifyPointerLeavedSurface(surface());
    QWaylandQuickItem::hoverLeaveEvent(event);
}

QWaylandSeat* WebOSSurfaceItem::getInputDevice(QInputEvent *event) const
{
#ifdef MULTIINPUT_SUPPORT
    if (!event)
        return nullptr;

    return m_compositor->seatFor(event);
#else
    if (!event || event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
        return m_compositor->keyboardDeviceForWindow(window());

    return m_compositor->defaultSeat();
#endif
}

void WebOSSurfaceItem::takeFocus(QWaylandSeat *device)
{
    QWaylandQuickItem::takeFocus(device ? device : getInputDevice());
}

void WebOSSurfaceItem::mouseUngrabEvent()
{
#ifdef MULTIINPUT_SUPPORT
    if (surface()) {
        QScopedPointer<QTouchEvent> e(new QTouchEvent(QEvent::TouchCancel));
        for (int i = 1; i < m_compositor->inputDevices().size(); i++) {
            QWaylandSeat *dev = m_compositor->inputDevices().at(i);
            if (!surface()->views().isEmpty() && dev && dev->mouseFocus() == surface()->views().first()) {
                e.reset(new QTouchEvent(QEvent::TouchCancel, Q_NULLPTR, (Qt::KeyboardModifiers)(static_cast<WebOSInputDevice*>(dev)->id())));
                break;
            }
        }
        QWaylandQuickItem::touchEvent(e.get());
    }
#else
    QWaylandQuickItem::mouseUngrabEvent();
#endif
}

void WebOSSurfaceItem::processKeyEvent(QKeyEvent *event)
{
    if (!surface()) {
        qWarning() << "Ignoring key event sent to surface item that has no surface" << this;
        return;
    }

    QWaylandSeat *inputDevice = getInputDevice(event);
    if (!inputDevice) {
        qWarning() << "no input device for this event";
        return;
    }

    auto keyboard = static_cast<WebOSKeyboard*>(inputDevice->keyboard());

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    // Make "autoRepeat" to "false" so that the event is handled
    // in QWaylandSeat::sendFullKeyEvent even when it was "true".
    // It is the policy we have been using in webOS for a long time.
    new (event) QKeyEvent(event->type(), event->key(), event->modifiers(),
            event->nativeScanCode(), event->nativeVirtualKey(), event->nativeModifiers(),
            event->text(), false /* autorep */);
#endif

    // General case
    if (!(isPartOfGroup() || isSurfaceGroupRoot()) ||
        // If keyboard is grabbed, do not propagate key events between
        // window-group to avoid unintended keyboard focus change.
        keyboard->currentGrab()) {

        if (hasFocus()) {
            qInfo() << "Focused none-group nor grabbed item: " << this << event->key();
            inputDevice->sendFullKeyEvent(event);
        } else {
            qInfo() << "Not focused none-group nor grabbed item: " << this << event->key();
        }
        return;
    }

    // Surface group
    if (acceptsKeyEvent(event)) {
        inputDevice->setKeyboardFocus(surface());
        inputDevice->sendFullKeyEvent(event);
    } else if (m_surfaceGroup) {
        WebOSSurfaceItem *nextItem = NULL;
        if (m_surfaceGroup->allowLayerKeyOrder()) {
            nextItem = m_surfaceGroup->nextKeyOrderedSurfaceGroupItem(this);
            qInfo() << this << "investigates next key ordered item: " << nextItem << event->key();
        }
        else {
            nextItem = m_surfaceGroup->nextZOrderedSurfaceGroupItem(this);
            qInfo() << this << "investigates next z ordered item: " << nextItem << event->key();
        }
        if (nextItem) {
            nextItem->processKeyEvent(event);
        }
        else {
            qInfo() << this << "got no next ordered item: " << event->key() << m_surfaceGroup->allowLayerKeyOrder();
        }
    } else {
        qInfo() << "Not surfaced: " << this << event->key();
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
    foreach (QWaylandSeat *dev, m_compositor->inputDevices()) {
        if (dev) {
            if (dev->keyboardFocus() == surface())
                dev->setKeyboardFocus(0);
            if (surface() && !surface()->views().isEmpty()
                && dev->mouseFocus() == surface()->views().first())
                dev->setMouseFocus(nullptr);
        }
    }
#else
    QWaylandSeat *keyboardDevice = getInputDevice();
    QWaylandSeat *mouseDevice = m_compositor->defaultSeat();
    if (keyboardDevice->keyboardFocus() == surface())
        keyboardDevice->setKeyboardFocus(0);
    if (surface() && mouseDevice->mouseFocus() == surface()->views().first())
        mouseDevice->setMouseFocus(nullptr);
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

    QVariantMap res;
    for (auto name: surface()->dynamicPropertyNames())
        res[name] = surface()->property(name.constData());

    return res;
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
        surface()->setProperty(key.toLatin1().constData(), value);
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
        if (surface())
            surface()->setObjectName(QString("surface_%1%2").arg(m_appId).arg(type()));
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
        if (surface())
            surface()->setObjectName(QString("surface_%1%2").arg(appId()).arg(m_type));
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

void WebOSSurfaceItem::setGroupedWindowModel(WebOSGroupedWindowModel* model)
{
    PMTRACE_FUNCTION;
    if (model != m_groupedWindowModel) {
        m_groupedWindowModel = model;
        emit groupedWindowModelChanged();
    }
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
        connect(m_shellSurface, &WebOSShellSurface::addonChanged,
                this, &WebOSSurfaceItem::addonChanged);
    }
}

void WebOSSurfaceItem::resetShellSurface(WebOSShellSurface *shell)
{
    PMTRACE_FUNCTION;
    if (shell && m_shellSurface == shell) {
        disconnect(m_shellSurface, SIGNAL(locationHintChanged()), this, SIGNAL(locationHintChanged()));
        disconnect(m_shellSurface, SIGNAL(keyMaskChanged()), this, SIGNAL(keyMaskChanged()));
        disconnect(m_shellSurface, SIGNAL(stateChangeRequested(Qt::WindowState)), this, SLOT(requestStateChange(Qt::WindowState)));
        disconnect(m_shellSurface, SIGNAL(propertiesChanged(QVariantMap, QString, QVariant)),
                   this, SLOT(updateProperties(QVariantMap, QString, QVariant)));
        m_shellSurface = Q_NULLPTR;
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

QString WebOSSurfaceItem::addon() const
{
    return m_shellSurface ? m_shellSurface->addon() : QStringLiteral("");
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
        QPointF newPos = mapToScene(QPointF(0, 0));
        if (newPos != m_position) {
            m_shellSurface->setPosition(newPos);
            m_position = newPos;
            emit positionUpdated();
        }
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

    if (window()) {
        WebOSCompositorWindow *w = static_cast<WebOSCompositorWindow *>(window());
        w->reportSurfaceDamaged(this);
    }

    /* Some surfaces can try to render after it is detached from scengraph.
       In that case, if compositor allows the rendering, compositor needs
       to call frameFinished. It will release previous front buffer
       of the surface. Otherwise, the surface will be stuck to wait for
       available buffer. */
    if (!window() && surface())
        surface()->sendFrameCallbacks();
}

void WebOSSurfaceItem::setNotifyPositionToClient(bool notify)
{

    if (m_notifyPositionToClient != notify) {
        m_notifyPositionToClient = notify;
        emit notifyPositionToClientChanged();
        if (m_notifyPositionToClient)
            updateScreenPosition();
    }
}

void WebOSSurfaceItem::setExposed(bool exposed)
{
    if (m_exposed != exposed) {
        if (m_shellSurface && surface()) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
            QRegion r = exposed ? QRegion(QRect(QPoint(0, 0), surface()->bufferSize())) : QRegion();
#else
            QRegion r = exposed ? QRegion(QRect(QPoint(0, 0), surface()->size())) : QRegion();
#endif
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
    case Qt::Key_webOS_BS_11:
    case Qt::Key_webOS_BS_12:
    case Qt::Key_webOS_CS1_11:
    case Qt::Key_webOS_CS1_12:
    case Qt::Key_webOS_CS2_11:
    case Qt::Key_webOS_CS2_12:
    case Qt::Key_webOS_TER_11:
    case Qt::Key_webOS_TER_12:
    case Qt::Key_webOS_4K_BS_11:
    case Qt::Key_webOS_4K_BS_12:
    case Qt::Key_webOS_4K_CS_11:
    case Qt::Key_webOS_4K_CS_12:
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
    case Qt::Key_webOS_InputTVRadio:
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
    case Qt::Key_webOS_BMLData:
        retKeyMask = KeyMaskData;
    break;
    case Qt::Key_webOS_Info:
        retKeyMask = KeyMaskRemoteMagnifierGroup | KeyMaskInfo;
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
                     ^ KeyMaskTeletextActiveGroup
                     ^ KeyMaskData
                     ^ KeyMaskInfo;
    break;
    }

    return retKeyMask;
}

void WebOSSurfaceItem::setCursorSurface(QWaylandSurface *surface, int hotSpotX, int hotSpotY)
{
    if (m_cursorView.surface() == surface && m_cursorHotSpotX == hotSpotX && m_cursorHotSpotY == hotSpotY) {
        qWarning() << "Cursor: attempting to set the same cursor surface, ignored" << surface << hotSpotX << hotSpotY;
    } else {
        if (!surface) {
            // Ignore null cursor surface because we use a different way to hide the cursor
            // by setting hotspot as 254. (see WebOSCoreCompositor::getCursor)
            qWarning() << "Cursor: null cursor has no effect in webOS";
            return;
        }

        qDebug() << "Cursor: updating cursor with surface" << surface << hotSpotX << hotSpotY;

        QCursor cursor;
        bool staticCursor = false;

        if (m_compositor->getCursor(surface, hotSpotX, hotSpotY, cursor)) {
            qDebug() << "Cursor: use the cursor designated by compositor" << cursor;
            staticCursor = true;
        } else {
            staticCursor = false;
        }

        if (m_cursorView.surface()) {
            qDebug() << "Cursor: disconnect old cursor surface" << m_cursorView.surface() << "static:" << staticCursor;
            QObject::disconnect(m_cursorView.surface(), SIGNAL(redraw()), this, SLOT(updateCursor()));
        }

        /* Original source changes m_cursorView.surface(), m_cursorHotSpotX and m_cursorHotSpotY even if getCursorFromSurface
         * failes. This is not quite right, I'm afraid, but unless told otherwise, I will preserve current behaviour.
         */
        m_cursorView.setSurface(surface);
        m_cursorHotSpotX = hotSpotX;
        m_cursorHotSpotY = hotSpotY;
        if (staticCursor) {
            qDebug() << "Cursor: set a static cursor" << cursor;
            setCursor(cursor);
        } else if (surface) {
            qDebug() << "Cursor: set a live cursor with cursor surface" << surface << "static:" << staticCursor;
            connect(surface, SIGNAL(redraw()), this, SLOT(updateCursor()), Qt::UniqueConnection);
        }
    }
}

void WebOSSurfaceItem::updateCursor()
{
    m_cursorView.advance();
    QImage image = m_cursorView.currentBuffer().image().copy();
    if (!image.isNull()) {
        if (m_cursorHotSpotX >= 0 && m_cursorHotSpotX <= image.size().width() &&
            m_cursorHotSpotY >= 0 && m_cursorHotSpotY <= image.size().height()) {
            qDebug() << "Cursor: live updating cursor with surface" << m_cursorView.surface() << m_cursorHotSpotX << m_cursorHotSpotY;
            QCursor c(QPixmap::fromImage(image), m_cursorHotSpotX, m_cursorHotSpotY);
            setCursor(c);
            return;
        }
    }
    qWarning() << "Cursor: fallback to the default cursor";
    setCursor(QCursor(Qt::ArrowCursor));
}

WebOSSurfaceItem *WebOSSurfaceItem::createMirrorItem()
{
    if (m_isMirrorItem) {
        qWarning() << "Cannot mirror a mirror item" << this;
        return nullptr;
    }

    WebOSSurfaceItem *mirror = new WebOSSurfaceItem(m_compositor, static_cast<QWaylandQuickSurface *>(surface()));
    mirror->m_isMirrorItem = true;
    mirror->m_mirrorSource = this;
    mirror->setObjectName(QString("mirrorItem_%1%2").arg(m_appId).arg(type()));
    if (mirror->surface())
        mirror->surface()->setObjectName(QString("mirrorSurface_%1%2").arg(m_appId).arg(type()));

    // Default setting for mirror item
    mirror->setEnabled(false);
    mirror->setDisplayAffinity(-1);

    qInfo() << "Mirror item created for" << this << mirror;

    foreach (WebOSExported *exported, m_exportedElements)
        exported->startImportedMirroring(mirror);

    m_mirrorItems.append(mirror);

    return mirror;
}

bool WebOSSurfaceItem::removeMirrorItem(WebOSSurfaceItem *mirror)
{
    if (!mirror)
        return false;

    mirror->m_mirrorSource = nullptr;

    return m_mirrorItems.removeOne(mirror);
}

bool WebOSSurfaceItem::hasSecuredContent()
{
    if (!isMapped())
        return false;

    if (view() && view()->currentBuffer().hasProtectedContent()) {
        qInfo() << "Current view has secured content";
        return true;
    }

    foreach (WebOSExported *exported, m_exportedElements) {
        if (exported->hasSecuredContent()) {
            qInfo() << "Exported item has secured content";
            return true;
        }
    }

    return false;
}

void WebOSSurfaceItem::setAddonStatus(AddonStatus status)
{
    if (m_shellSurface)
        m_shellSurface->setAddonStatus(status);
}

QJSValue WebOSSurfaceItem::addonFilter() const
{
    return m_addonFilter;
}

void WebOSSurfaceItem::setAddonFilter(const QJSValue &filter)
{
    if (!filter.isCallable()) {
        qWarning() << "addonFilter must be a callable function";
        return;
    }

    if (!m_addonFilter.strictlyEquals(filter)) {
        m_addonFilter = filter;
        emit addonFilterChanged();
    }
}

bool WebOSSurfaceItem::acceptsAddon(const QString &newAddon)
{
    bool accepts = false;

    if (!addonFilter().isUndefined()) {
        QJSValueList args;
        args.append(QJSValue(newAddon));
        QJSValue ret = addonFilter().call(args);
        if (ret.isBool())
            accepts = ret.toBool();
        else
            qWarning() << "addonFilter does not return bool for" << newAddon;
    } else {
        qWarning() << "addonFilter is undefined for" << newAddon;
    }

    qInfo() << (accepts ? "Accepted" : "Denied") << "addon" << newAddon;

    return accepts;
}

void WebOSSurfaceItem::grabLastFrame()
{
    if (surface()) {
        qDebug() << "Grabbing surface for item" << this << "with its buffer locked";
        setBufferLocked(true);
        m_surfaceGrabbed = surface();
        QWaylandSurfacePrivate::get(m_surfaceGrabbed)->ref();
    } else {
        qWarning() << "Attempting to grab the last frame of the item unsurfaced" << this;
    }
}

void WebOSSurfaceItem::releaseLastFrame()
{
    if (m_surfaceGrabbed) {
        qDebug() << "Releasing surface for item" << this;
        setBufferLocked(false);
        if (!isMapped()) {
            qDebug() << "Confirmed surface is unmapped, handling surfaceUnmapped for item" << this;
            m_compositor->handleSurfaceUnmapped(this);
        }
        QWaylandSurface *s = m_surfaceGrabbed;
        m_surfaceGrabbed = nullptr;
        // Make sure nothing to be done for the surface item from this point
        // since dereferencing of the surface would lead this item to be destroyed
        // if refCount of the surface becomes 0
        QWaylandSurfacePrivate::get(s)->deref();
    } else {
        qWarning() << "No surface grabbed and thus nothing to release for" << this;
    }
}

void WebOSSurfaceItem::surfaceChangedEvent(QWaylandSurface *newSurface, QWaylandSurface *oldSurface)
{
    if (m_surfaceGrabbed && m_surfaceGrabbed == oldSurface) {
        qDebug() << "Change m_surfaceGrabbed" << m_surfaceGrabbed << "to newSurface" << newSurface;
        QWaylandSurfacePrivate::get(m_surfaceGrabbed)->deref();
        m_surfaceGrabbed = newSurface;
    }

    pid_t pid;
    uid_t uid;

    // Look more info here: https://doc.qt.io/qt-5/qwaylandclient.html
    struct wl_client *client = surface() && surface()->client() ? surface()->client()->client() : nullptr;
    if (client) {
        wl_client_get_credentials(client, &pid, &uid, 0);
    } else {
        pid = getpid();
        uid = getuid();
    }

    QString processId = QStringLiteral("%1").arg(pid);
    if (m_processId != processId) {
        m_processId = processId;
        emit processIdChanged();
    }

    QString userId = QStringLiteral("%1").arg(uid);
    if (m_userId != userId) {
        m_userId = userId;
        emit userIdChanged();
    }

    QWaylandQuickItem::surfaceChangedEvent(newSurface, oldSurface);
}

uint32_t WebOSSurfaceItem::planeZpos() const
{
    // FIXME
    // Values copied from the underlying platform abstraction code
    // as no proper header file exists for the definition.
    enum PlaneOrder {
        VideoPlane = 0,
        FullscreenPlane = 1,
        MainPlane = 2,
        Plane_End = 3
    };

    if (m_imported)
        return VideoPlane;

    if (!directUpdateOnPlane()) {
        qWarning() << "This will should not happen for planeZpos of MainPlane" << this;
        return MainPlane;
    }

    return FullscreenPlane;
}

void WebOSSurfaceItem::updateDirectUpdateOnPlane()
{
    WebOSSurfaceItem *wItem = qobject_cast<WebOSSurfaceItem *>(sender());
    if (!wItem || !m_imported)
        return;

    qInfo() << "updateDirectUpdateOnPlane" << this << "by" << wItem;

    setDirectUpdateOnPlane(wItem->directUpdateOnPlane());
}

bool WebOSSurfaceItem::directUpdateOnPlane() const
{
    return m_directUpdateOnPlane;
}

void WebOSSurfaceItem::setDirectUpdateOnPlane(bool enable)
{
    if (m_directUpdateOnPlane != enable) {
        m_directUpdateOnPlane = enable;

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        qInfo() << m_directUpdateOnPlane << this;
        if (m_directUpdateOnPlane) {
            m_hardwarelayer = new QWaylandQuickHardwareLayer(this);
            m_hardwarelayer->setStackingLevel(planeZpos());
            m_hardwarelayer->initialize();
        } else {
            delete m_hardwarelayer;
            m_hardwarelayer = nullptr;
        }
#endif
        emit directUpdateOnPlaneChanged();
    }
}


#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
QSGNode *WebOSSurfaceItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    QWaylandBufferRef ref = view()->currentBuffer();

    if (directUpdateOnPlane() && ref.directUpdate(this, planeZpos())) {
        QQuickWindowPrivate *d = QQuickWindowPrivate::get(window());
        emit d->renderer->sceneGraphChanged();
        // This is needed when rendering mode is changed from general texture to direct update
        delete oldNode;
        return nullptr;
    }

    return QWaylandQuickItem::updatePaintNode(oldNode, data);
}
#endif
