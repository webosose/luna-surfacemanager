// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#include <qwaylandsurfaceitem.h>
#include <QWaylandInputDevice>
#include <QtCompositor/private/qwlkeyboard_p.h>
#include <QtCompositor/private/qwlinputdevice_p.h>
#include <QtCompositor/private/qwlcompositor_p.h>

#include <QDebug>
#include <QQuickWindow>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QQmlComponent>
#include <QProcess>

#include "weboscorecompositor.h"
#include "weboscompositorwindow.h"
#include "weboswindowmodel.h"
#include "webosgroupedwindowmodel.h"
#include "webossurfacemodel.h"
#include "webossurfaceitem.h"
#include "webossurfacegroup.h"
#include "webosshellsurface.h"
#ifdef MULTIINPUT_SUPPORT
#include "webosinputdevice.h"
#endif
#include "webosseat.h"
#include "webosinputmethod.h"
#include "compositorextension.h"
#include "compositorextensionfactory.h"

#include "webossurfacegroupcompositor.h"
#include "webosscreenshot.h"

// Needed extra for type registration
#include "weboskeyfilter.h"
#include "webosinputmethod.h"

#include "weboscompositortracer.h"

// Need to access QtWayland::Keyboard::focusChanged
#include <QtCompositor/private/qwlsurface_p.h>

// Specify qtwayland extensions as needed.
// Refer to qwaylandcompositor.h for available extensions.
static QWaylandCompositor::ExtensionFlags compositorFlags = 0;

#ifdef USE_PMLOGLIB
#include <PmLogLib.h>
#endif

void WebOSCoreCompositor::logger(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const char* function  = context.function;
    const char* userMessage  = message.toUtf8().constData();

#ifdef USE_PMLOGLIB
    static PmLogContext pmLogCtx;
    static bool hasPmLogCtx = false;
    static const char* ID = "LSM";

    if (Q_UNLIKELY(!hasPmLogCtx)) {
        PmLogGetContext("surface-manager", &pmLogCtx);
        hasPmLogCtx = true;
    }

    switch (type) {
        case QtDebugMsg:
            PmLogDebug(pmLogCtx, "%s, %s", function, userMessage);
            break;
        case QtInfoMsg:
            PmLogInfo(pmLogCtx, ID, 0, "%s, %s", function, userMessage);
            break;
        case QtWarningMsg:
            PmLogWarning(pmLogCtx, ID, 0, "%s, %s", function, userMessage);
            break;
        case QtCriticalMsg:
            PmLogError(pmLogCtx, ID, 0, "%s, %s", function, userMessage);
            break;
        case QtFatalMsg:
            PmLogCritical(pmLogCtx, ID, 0, "%s, %s", function, userMessage);
            break;
    }
#else
    static const char* proc = QString(QFileInfo(QCoreApplication::applicationFilePath()).fileName()).toLatin1().constData();
    switch (type) {
        case QtDebugMsg:
            printf("[%s|DEBUG   ] %s :: %s\n", proc, function, userMessage);
            break;
        case QtInfoMsg:
            printf("[%s|INFO    ] %s :: %s\n", proc, function, userMessage);
            break;
        case QtWarningMsg:
            printf("[%s|WARNING ] %s :: %s\n", proc, function, userMessage);
            break;
        case QtCriticalMsg:
            printf("[%s|CRITICAL] %s :: %s\n", proc, function, userMessage);
            break;
        case QtFatalMsg:
            printf("[%s|FATAL   ] %s :: %s\n", proc, function, userMessage);
            break;
    }
#endif
}

WebOSCoreCompositor::WebOSCoreCompositor(ExtensionFlags extensions, const char *socketName)
    : QWaylandQuickCompositor(socketName, compositorFlags)
    , m_previousFullscreenSurface(0)
    , m_fullscreenSurface(0)
    , m_keyFilter(0)
    , m_cursorVisible(false)
    , m_mouseEventEnabled(true)
    , m_shell(0)
    , m_acquired(false)
    , m_directRendering(false)
    , m_fullscreenTick(0)
    , m_surfaceGroupCompositor(0)
    , m_unixSignalHandler(new UnixSignalHandler(this))
    , m_eventPreprocessor(new EventPreprocessor(this))
    , m_inputMethod(0)
    , m_surfaceItemClosePolicy(QVariantMap())
#ifdef MULTIINPUT_SUPPORT
    , m_lastMouseEventFrom(0)
#endif
{
    qInfo() << "Creating WebOSCoreCompositor with flags" << compositorFlags;

    checkWaylandSocket();

    initializeExtensions(extensions);
}

WebOSCoreCompositor::~WebOSCoreCompositor()
{
}


void WebOSCoreCompositor::registerWindow(QQuickWindow *window, QString name)
{
    static bool firstRegister = true;

    QWaylandOutput *output = createOutput(window, "webos", name);

    if (!output) {
        qCritical() << "Failed to create QWaylandOutput for window" << window << name;
        return;
    }

    m_outputs.insert(window, output);

    connect(window, SIGNAL(frameSwapped()), this, SLOT(frameSwappedSlot()));
    //TODO: check is it ok just to use primary window to handle activeFocusItem
    connect(window, SIGNAL(activeFocusItemChanged()), this, SLOT(handleActiveFocusItemChanged()));

    if (firstRegister) {
        firstRegister = false;

        //For QtWayland shell surface in 5.4
        addDefaultShell();

        m_surfaceModel = new WebOSSurfaceModel();

        setInputMethod(new WebOSInputMethod(this));

        connect(this, SIGNAL(fullscreenSurfaceChanged()), this, SIGNAL(fullscreenChanged()));

        connect(m_unixSignalHandler, &UnixSignalHandler::sighup, this, &WebOSCoreCompositor::reloadConfig);

        connect(defaultInputDevice()->handle()->keyboardDevice(), &QtWayland::Keyboard::focusChanged, this, &WebOSCoreCompositor::activeSurfaceChanged);

        QCoreApplication::instance()->installEventFilter(m_eventPreprocessor);

        m_extensions = CompositorExtensionFactory::create(this);

        m_shell = new WebOSShell(this);

        m_inputManager = new WebOSInputManager(this);
#ifdef MULTIINPUT_SUPPORT
        m_inputDevicePreallocated = new WebOSInputDevice(this);
#endif

        // Set default state of Qt client windows to fullscreen
        setClientFullScreenHint(true);

        emit surfaceModelChanged();
        emit windowChanged();
    }
}

void WebOSCoreCompositor::checkWaylandSocket() const
{
    QByteArray name(socketName());
    // Similar logic as in wl_display_add_socket()
    if (name.isEmpty()) {
        name = qgetenv("WAYLAND_DISPLAY");
        if (name.isEmpty())
            name = "wayland-0";
    }

    QFileInfo sInfo(QString("%1/%2").arg(qgetenv("XDG_RUNTIME_DIR").constData()).arg(name.data()));
    QFileInfo dInfo(sInfo.absoluteDir().absolutePath());
    if (QFile::setPermissions(sInfo.absoluteFilePath(),
                QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                QFileDevice::ReadGroup | QFileDevice::WriteGroup)) {
        // TODO: Qt doesn't provide a method to chown at the moment
        QString cmd = QString("/bin/chown %1:%2 %3").arg(dInfo.owner()).arg(dInfo.group()).arg(sInfo.absoluteFilePath());
        qDebug() << "Setting ownership of" << sInfo.absoluteFilePath() << "by" << cmd;
        QProcess::startDetached(cmd);
    } else {
        qCritical() << "Unable to set permission for" << sInfo.absoluteFilePath();
    }
}

void WebOSCoreCompositor::registerTypes()
{
    qmlRegisterType<WebOSWindowModel>("WebOSCoreCompositor", 1,0, "WindowModel");
    qmlRegisterType<WebOSGroupedWindowModel>("WebOSCoreCompositor", 1,0, "GroupedWindowModel");
    qmlRegisterType<WebOSSurfaceModel>("WebOSCoreCompositor", 1, 0, "SurfaceModel");
    qmlRegisterUncreatableType<WebOSSurfaceItem>("WebOSCoreCompositor", 1, 0, "SurfaceItem", QLatin1String("Not allowed to create SurfaceItem"));
    qmlRegisterType<WebOSKeyFilter>("WebOSCoreCompositor", 1, 0, "KeyFilter");
    qmlRegisterType<WebOSInputMethod>("WebOSCoreCompositor", 1, 0, "InputMethod");
    qmlRegisterType<WebOSSurfaceGroup>("WebOSCoreCompositor", 1, 0, "SurfaceItemGroup");
    qmlRegisterType<WebOSScreenShot>("WebOSCoreCompositor", 1, 0, "ScreenShot");
    qmlRegisterUncreatableType<WebOSKeyPolicy>("WebOSCoreCompositor", 1, 0, "KeyPolicy", QLatin1String("Not allowed to create KeyPolicy instance"));
}

QWaylandQuickSurface* WebOSCoreCompositor::fullscreenSurface() const
{
    return m_fullscreenSurface;
}

WebOSSurfaceModel* WebOSCoreCompositor::surfaceModel() const
{
    return m_surfaceModel;
}

WebOSInputMethod* WebOSCoreCompositor::inputMethod() const
{
    return m_inputMethod;
}

void WebOSCoreCompositor::setInputMethod(WebOSInputMethod* inputMethod)
{
    if (m_inputMethod != inputMethod) {
        if (m_inputMethod != NULL) {
            m_inputMethod->disconnect();
            delete m_inputMethod;
        }

        m_inputMethod = inputMethod;

        connect(m_inputMethod, SIGNAL(inputMethodActivated()), this, SIGNAL(inputPanelRequested()));
        connect(m_inputMethod, SIGNAL(inputMethodDeactivated()), this, SIGNAL(inputPanelDismissed()));

        emit inputMethodChanged();
    }
}

bool WebOSCoreCompositor::isMapped(WebOSSurfaceItem *item)
{
    if (webOSWindowExtension())
        return item->surface() && m_surfaceModel->indexFromItem(item).isValid();
    else
        return item->surface() && m_surfaces.contains(item);
}
/*
 * update our internal model of mapped surface in response to wayland surfaces being mapped
 * and unmapped. WindowModels use this as their source of windows.
 */
void WebOSCoreCompositor::onSurfaceMapped() {
    PMTRACE_FUNCTION;
    QWaylandQuickSurface *surface = qobject_cast<QWaylandQuickSurface *>(sender());
    // qtwayland project uses the wayland-cursor library to provide a distinct surface to
    // which to render on, so we check if the surface has a shell surface associated with it as
    // we do not want to list the cursor surface with the 'normal' ones.
    WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem*>(surface->surfaceItem());

    if (item) {
        if (item->isPartOfGroup()) {
            // The management of surface groups is left solely to the qml
            // for example the state changes etc. This is done to ensure that
            // surfaces belonging to a group do not affect the lifecycle of the
            // current active fullscreen surface.
            //
            // NOTE: subject to change during implementation!!!
            qDebug() << "Mapping surface " << item << "group" << item->surfaceGroup()->name();
        }

        qInfo() << surface << item << item->appId() << item->type();

        // There might be a proxy item for this, so lets make sure the we get rid of it before
        // adding the new one into the models
        deleteProxyFor(item);

        //All mapped surface should have ItemStateNormal state.
        item->setItemState(WebOSSurfaceItem::ItemStateNormal);
        if (webOSWindowExtension()) {
            m_surfaceModel->surfaceMapped(item);
            if (!m_surfaces.contains(item)) {
                m_surfaces << item;
            } else {
                qDebug() << item << "This item already exists in m_surfaces.";
            }
        } else {
            // if item is still in m_surfaces after deleteProxyFor, it's not a proxy
            // but a normal item
            if (!m_surfaces.contains(item)) {
                m_surfaceModel->surfaceMapped(item);
                m_surfaces << item;
            }
        }

        qDebug() << item << "Items in compositor: " <<  getItems();
        emit surfaceMapped(item);
    }
}

WebOSSurfaceItem* WebOSCoreCompositor::activeSurface()
{
    QWaylandSurface* active = defaultInputDevice()->keyboardFocus();
    if (!active)
        return 0;

    Q_ASSERT(active->views().first());
    QWaylandSurfaceItem *item = static_cast<QWaylandSurfaceItem*>(active->views().first());
    return qobject_cast<WebOSSurfaceItem*>(item);
}

void WebOSCoreCompositor::onSurfaceUnmapped() {
    PMTRACE_FUNCTION;
    QWaylandQuickSurface *surface = qobject_cast<QWaylandQuickSurface *>(sender());

    WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem*>(surface->surfaceItem());

    if (!item) {
        qWarning() << "No item for surface" << surface;
        return;
    }

    qInfo() << surface << item << item->appId() << item->itemState();

    if (webOSWindowExtension()) {
        processSurfaceItem(item, WebOSSurfaceItem::ItemStateHidden);
    } else {
        if (item->itemState() != WebOSSurfaceItem::ItemStateProxy) {
            m_surfaceModel->surfaceUnmapped(item);
            emit surfaceUnmapped(item);
            m_surfaces.removeOne(item);
        } else {
            emit surfaceUnmapped(item); //We have to notify qml even for proxy item
        }
    }
}

void WebOSCoreCompositor::onSurfaceDestroyed() {
    PMTRACE_FUNCTION;
    QWaylandQuickSurface *surface = qobject_cast<QWaylandQuickSurface *>(sender());

    WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem*>(surface->surfaceItem());

    if (!item) {
        qWarning() << "No item for surface" << surface;
        return;
    }

    qInfo() << surface << item << item->appId() << item->itemState() << item->itemStateReason();

    if (webOSWindowExtension()) {
        processSurfaceItem(item, WebOSSurfaceItem::ItemStateProxy);
    } else {
        if (item->itemState() != WebOSSurfaceItem::ItemStateProxy) {
            m_surfacesOnUpdate.removeOne(item);
            removeSurfaceItem(item, true);
        } else {
            emit surfaceDestroyed(item); //We have to notify qml even for proxy item
            // This means items will not use any graphic resource from related surface.
            // If there are more use case, the API should be moved to proper place.
            // ex)If there are some dying animation for the item, this should be called at the end of the animation.
            item->releaseSurface();
            // Clear old texture
            item->updateTexture();
            item->update();
        }

        if (surface == m_fullscreenSurface)
            setFullscreenSurface(NULL);
    }
}

void WebOSCoreCompositor::frameSwappedSlot() {
    PMTRACE_FUNCTION;
    QWindow *window = qobject_cast<QWindow *>(sender());
    QList<QWaylandSurface *> ss;
    foreach (QWaylandSurface *s, surfaces())
        if (s->output()->window() == window)
            ss << s;
    sendFrameCallbacks(ss);
}

/* Basic life cycle of surface and surface item.
   1. new QtWayland::Surface { new QWaylandSurface }
        -> QWebOSCoreCompositor::surfaceCreated { new QWebOSSurfaceItem }

   2. delete QtWayland::Surface { delete QWaylandSurface {
        ~QObject() {
         emit destroyed(QObject *) -> WebOSCoreCompositor::surfaceDestroyed() / QWaylandSurfaceItem::surfaceDestroyed()
         deleteChilderen -> ~QWaylandSurfacePrivate()
        }
      }}

   We will delete QWebOSSurfaceItem in WebOSCoreCompositor::surfaceDestroyed as a pair of surfaceCreated.
   For that, we have to guarantee WebOSCoreCompositor::surfaceDestroyed is called before QWaylandSurfaceItem::surfaceDestroyed.
   Otherwise, the surface item will lose a reference for surface in QWaylandSurfaceItem::surfaceDestroyed.
   After all, ~QWebOSSurfaceItem cannot clear surface's reference, then the surface still have invaild reference to
   surface item which is already freed. */

void WebOSCoreCompositor::surfaceCreated(QWaylandSurface *surface) {
    PMTRACE_FUNCTION;
    // There are two surfaceDestroyed functions (a slot and a signal) need to
    // cast to point to the right one, otherwise compilation fails
    connect(surface, &QWaylandSurface::surfaceDestroyed, this, &WebOSCoreCompositor::onSurfaceDestroyed);
    connect(surface, SIGNAL(mapped()), this, SLOT(onSurfaceMapped()));
    connect(surface, SIGNAL(unmapped()), this, SLOT(onSurfaceUnmapped()));

    /* Ensure that WebOSSurfaceItem is created after surfaceDestroyed is connected.
       Refer to upper comment about life cycle of surface and surface item. */
    WebOSSurfaceItem *item = new WebOSSurfaceItem(this, static_cast<QWaylandQuickSurface *>(surface));
    // ensure that the item will not resize by default. QtWayland is missing a inline
    // initializer for the member variable in the contructor
    item->setResizeSurfaceToItem(false);
    qInfo() << surface << item << "client pid:" << item->processId();
}

WebOSSurfaceItem* WebOSCoreCompositor::createProxyItem(const QString& appId, const QString& title, const QString& subtitle, const QString& snapshotPath)
{
    WebOSSurfaceItem *item = new WebOSSurfaceItem(this, NULL);
    item->setAppId(appId);
    item->setTitle(title);
    item->setSubtitle(subtitle);
    item->setCardSnapShotFilePath(snapshotPath);

    item->setItemState(WebOSSurfaceItem::ItemStateProxy);
    /* To be in recent model */
    item->setLastFullscreenTick(getFullscreenTick());
    /* Add into recent list */
    m_surfaceModel->surfaceMapped(item);
    /* To be deleted when launched in deleteProxyFor */
    m_surfaces << item;

    return item;
}

void WebOSCoreCompositor::deleteProxyFor(WebOSSurfaceItem* newItem)
{
    PMTRACE_FUNCTION;
    QMutableListIterator<WebOSSurfaceItem*> si(m_surfaces);
    while (si.hasNext()) {
        WebOSSurfaceItem* item = si.next();
        if (item->itemState() == WebOSSurfaceItem::ItemStateProxy
                && newItem->appId() == item->appId()
                && newItem != item) { //we don't want to remove item for mapped surface
            qDebug() << "deleting proxy" << item << " for newItem" << newItem;
            si.remove();
            m_surfaceModel->surfaceDestroyed(item);
            emit surfaceDestroyed(item);
            delete item;
        }
    }
}

void WebOSCoreCompositor::removeSurfaceItem(WebOSSurfaceItem* item, bool emitSurfaceDestroyed)
{
    PMTRACE_FUNCTION;

    m_surfaceModel->surfaceDestroyed(item);
    if (emitSurfaceDestroyed)
        emit surfaceDestroyed(item);
    m_surfaces.removeOne(item);
    delete item;
}

WebOSSurfaceItem* WebOSCoreCompositor::fullscreen() const
{
    return m_fullscreenSurface ? qobject_cast<WebOSSurfaceItem*>(m_fullscreenSurface->surfaceItem()) : NULL;
}

void WebOSCoreCompositor::setFullscreen(WebOSSurfaceItem* item)
{
    setFullscreenSurface(item ? item->surface() : NULL);
}

bool WebOSCoreCompositor::setFullscreenSurface(QWaylandSurface *s) {
    PMTRACE_FUNCTION;
    QWaylandQuickSurface* surface = qobject_cast<QWaylandQuickSurface *>(s);
    // NOTE: Some surface is not QWaylandQuickSurface (e.g. Cursor), we still need more attention in that case.
    // TODO Move the block to qml
    if (!surface) {
        emit homeScreenExposed();
    }

    if (surface != m_fullscreenSurface) {
        m_inputMethod->deactivate();
        // The notion of the fullscreen surface needs to remain here for now as
        // direct rendering support needs a handle to it
        if (surface == NULL) {
            m_previousFullscreenSurface = NULL;
        } else {
            m_previousFullscreenSurface = m_fullscreenSurface;
        }
        m_fullscreenSurface = surface;
        emit fullscreenSurfaceChanged();
        // This is here for a transitional period, see signal comments
        emit fullscreenSurfaceChanged(m_previousFullscreenSurface, m_fullscreenSurface);
    }

    // TODO: Clean up the return type
    return true;
}

void WebOSCoreCompositor::surfaceAboutToBeDestroyed(QWaylandSurface *s)
{
    PMTRACE_FUNCTION;
    QWaylandQuickSurface* surface = qobject_cast<QWaylandQuickSurface *>(s);
    // NOTE: Some surface is not QWaylandQuickSurface (e.g. Cursor), we still need more attention in that case.
    if (surface && surface->surfaceItem()) {
        WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem *>(surface->surfaceItem());
        emit surfaceAboutToBeDestroyed(item);
    }
}

void WebOSCoreCompositor::closeWindow(QVariant window, QJSValue payload)
{
    PMTRACE_FUNCTION;
    WebOSSurfaceItem* item = qvariant_cast<WebOSSurfaceItem *>(window);
    if (!item) {
        qWarning() << "called with null or not a surface, ignored.";
        return;
    }

    item->deleteSnapShot();
    item->setCardSnapShotFilePath(NULL);

    if (webOSWindowExtension()) {
        QJsonObject jsp = QJsonObject::fromVariantMap(payload.toVariant().toMap());
        webOSWindowExtension()->windowClose()->close(item, jsp);
    } else {
        item->setItemState(WebOSSurfaceItem::ItemStateClosing);
        if (item->surface() && item->surface()->client()) {
            item->close();
        } else {
            removeSurfaceItem(item, false);
        }
    }
}

// NOTE: After Unified App Lifecycle Management,
// This function may not be called anywhere.
// However, we will remain this function to keep backward compatibility.
void WebOSCoreCompositor::closeWindowKeepItem(QVariant window)
{
    PMTRACE_FUNCTION;
    WebOSSurfaceItem* item = qvariant_cast<WebOSSurfaceItem *>(window);
    if (!item) {
        qWarning() << "called with null or not a surface, ignored.";
        return;
    }

    if (webOSWindowExtension()) {
        webOSWindowExtension()->windowClose()->close(item);
    } else {
        // Set as proxy unless marked as closing
        if (item->itemState() != WebOSSurfaceItem::ItemStateClosing)
            item->setItemState(WebOSSurfaceItem::ItemStateProxy);
        if (item->surface() && item->surface()->client())
            item->close();
    }
}

CompositorExtension *WebOSCoreCompositor::webOSWindowExtension()
{
    return m_extensions.value("webos-window-extension");
}

bool WebOSCoreCompositor::checkSurfaceItemClosePolicy(const QString &reason, WebOSSurfaceItem *item)
{
    PMTRACE_FUNCTION;

    QVariantMap itemClosePolicy = item->closePolicy();
    if (itemClosePolicy.contains(reason))
        return itemClosePolicy.value(reason).toBool();

    return m_surfaceItemClosePolicy.value(reason).toBool();
}

WebOSSurfaceItem* WebOSCoreCompositor::getSurfaceItemByAppId(const QString& appId)
{
    QMutableListIterator<WebOSSurfaceItem*> si(m_surfaces);
    while (si.hasNext()) {
        WebOSSurfaceItem* item = si.next();
        if (item->appId() == appId) {
            return item;
        }
    }
    return NULL;
}

void WebOSCoreCompositor::applySurfaceItemClosePolicy(QString reason, const QString &targetAppId)
{
    PMTRACE_FUNCTION;

    // if the reason does not exist, use a default reason
    if (!m_surfaceItemClosePolicy.contains(reason)) {
        qWarning() << "Closed reason does not valid: " << reason << "Reason will be used default one(undefined)";
        reason = "undefined";
    }

    WebOSSurfaceItem *item = getSurfaceItemByAppId(targetAppId);

    // We assume that the following logic is only valid for an app which satisfies the following conditions:
    // case 1. it has an app. Id
    // case 2. it is already mapped.
    // Otherwise, the surface item will be processed in the onSurfaceUnmapped and onSurfaceDestroyed properly.
    if (!item) {
        qDebug() << "The surface already unmapped: " << targetAppId;
        return;
    }

    qInfo() << "reason set, processing item" << item << item->itemState() << item->itemStateReason();
    item->setItemStateReason(reason);
    processSurfaceItem(item, item->itemState());
}

void WebOSCoreCompositor::processSurfaceItem(WebOSSurfaceItem* item, WebOSSurfaceItem::ItemState stateToBe)
{
    PMTRACE_FUNCTION;

    qDebug() << item << item->itemState() << item->itemStateReason();

    switch (stateToBe) {
    case WebOSSurfaceItem::ItemStateNormal:
        qWarning() << "no transition to ItemStateNormal for" << item << item->itemState() << item->itemStateReason();
        break;
    case WebOSSurfaceItem::ItemStateHidden:
        if (item->itemState() == WebOSSurfaceItem::ItemStateNormal &&
            (!item->isSurfaced() || !item->surface()->isMapped())) {
            qInfo() << "transitioning to ItemStateHidden for" << item << item->itemState() << item->itemStateReason();
            item->setItemState(WebOSSurfaceItem::ItemStateHidden, item->itemStateReason());

            m_surfaceModel->surfaceUnmapped(item);
            emit surfaceUnmapped(item);
        } else {
            qWarning() << "no transition to ItemStateHidden for" << item << item->itemState() << item->itemStateReason() << item->isSurfaced();
        }
        if (item->itemState() == WebOSSurfaceItem::ItemStateHidden && !item->itemStateReason().isEmpty()) {
            // reset state reason to re-use
            item->unsetItemStateReason();
        }
        break;
    case WebOSSurfaceItem::ItemStateProxy:
        if ((item->itemState() == WebOSSurfaceItem::ItemStateNormal || item->itemState() == WebOSSurfaceItem::ItemStateHidden) &&
            (!item->isSurfaced() || !item->surface()->isMapped())) {
            if (!item->itemStateReason().isEmpty() && checkSurfaceItemClosePolicy(item->itemStateReason(), item)) {
                qInfo() << "transitioning to ItemStateProxy and ItemStateClosing for" << item << item->itemState() << item->itemStateReason();
                item->setItemState(WebOSSurfaceItem::ItemStateProxy, item->itemStateReason());
                processSurfaceItem(item, WebOSSurfaceItem::ItemStateClosing);
            } else {
                qInfo() << "transitioning to ItemStateProxy for" << item << item->itemState() << item->itemStateReason();
                item->setItemState(WebOSSurfaceItem::ItemStateProxy, item->itemStateReason());

                emit surfaceDestroyed(item);

                /* This means items will not use any graphic resource from related surface.
                / If there are more use case, the API should be moved to proper place.
                / ex)If there are some dying animation for the item, this should be called at the end of the animation. */
                item->releaseSurface();
                // Clear old texture
                item->update();
            }
        } else {
            qWarning() << "no transition to ItemStateProxy for" << item << item->itemState() << item->itemStateReason() << item->isSurfaced();
        }
        break;
    case WebOSSurfaceItem::ItemStateClosing:
        if (!item->isSurfaced() || !item->surface()->isMapped()) {
            qInfo() << "transitioning to ItemStateClosing for" << item << item->itemState() << item->itemStateReason();
            item->setItemState(WebOSSurfaceItem::ItemStateClosing, item->itemStateReason());
        }
        if (item->itemState() == WebOSSurfaceItem::ItemStateClosing) {
            // remove item
            removeSurfaceItem(item, true);
        }
        break;
    default:
        break;
    }
}

void WebOSCoreCompositor::destroyClientForWindow(QVariant window) {
    QWaylandSurface *surface = qobject_cast<QWaylandSurfaceItem *>(qvariant_cast<QObject *>(window))->surface();
    destroyClientForSurface(surface);
}

bool WebOSCoreCompositor::getCursor(QWaylandSurface *surface, int hotSpotX, int hotSpotY, QCursor& cursor)
{
    // webOS specific cursor handling with reserved hotspot values
    // 1) 255: ArrowCursor
    // 2) 254: BlankCursor
    if (hotSpotX == 255 && hotSpotY == 255) {
        cursor = QCursor(Qt::ArrowCursor);
        return true;
    } else if (hotSpotX == 254 && hotSpotY == 254) {
        cursor = QCursor(Qt::BlankCursor);
        return true;
    }
    return false;
}

void WebOSCoreCompositor::setCursorSurface(QWaylandSurface *surface, int hotspotX, int hotspotY, WaylandClient *client)
{
    PMTRACE_FUNCTION;
    if (surface) {
        QWaylandQuickSurface *qs = static_cast<QWaylandQuickSurface *>(surface);
        disconnect(static_cast<QQuickWindow*>(qs->output()->window()), &QQuickWindow::beforeSynchronizing, qs, &QWaylandQuickSurface::updateTexture);
        disconnect(static_cast<QQuickWindow*>(qs->output()->window()), &QQuickWindow::sceneGraphInvalidated, qs, &QWaylandQuickSurface::invalidateTexture);
    }

    foreach(WebOSSurfaceItem *item, m_surfaces) {
        if (item->surface() && !item->surface()->handle()->isCursorSurface() && item->surface()->client() == client)
            item->setCursorSurface(surface, hotspotX, hotspotY);
    }
}

/* If HomeScreen is visible App cannot get any events.
 * So send pointer leave/enter event to fullscreen surface. */
void WebOSCoreCompositor::setMouseFocus(QWaylandSurface* surface)
{
    PMTRACE_FUNCTION;
    if (!surface)
        static_cast<WebOSCompositorWindow *>(window())->setDefaultCursor();

    QWaylandQuickSurface *qsurface = static_cast<QWaylandQuickSurface *>(surface);
#ifdef MULTIINPUT_SUPPORT
    QPointF curPosition = static_cast<QPointF>(QCursor::pos());
    foreach (QWaylandInputDevice *dev, inputDevices()) {
        if (surface && !surface->views().isEmpty())
            dev->setMouseFocus(surface->views().first(), curPosition, curPosition);
    }
#else
    QPointF curPosition = static_cast<QPointF>(QCursor::pos());
    defaultInputDevice()->setMouseFocus(qsurface->surfaceItem(), curPosition, curPosition);
#endif
}

#ifdef MULTIINPUT_SUPPORT
void WebOSCoreCompositor::resetMouseFocus(QWaylandSurface *surface)
{
    QPointF curPosition = static_cast<QPointF>(QCursor::pos());
    foreach (QWaylandInputDevice *dev, inputDevices()) {
        if (surface && !surface->views().isEmpty()
            && dev->mouseFocus() == surface->views().first())
            dev->setMouseFocus(NULL, curPosition, curPosition);
    }
}
#endif

WebOSKeyFilter* WebOSCoreCompositor::keyFilter() const
{
    return const_cast<WebOSKeyFilter*>(m_keyFilter);
}

void WebOSCoreCompositor::setKeyFilter(WebOSKeyFilter *filter)
{
    PMTRACE_FUNCTION;
    if (m_keyFilter != filter) {
        window()->removeEventFilter(m_keyFilter);
        window()->installEventFilter(filter);

        foreach (CompositorExtension* ext, m_extensions) {
            ext->removeEventFilter(m_keyFilter);
            ext->installEventFilter(filter);
        }
        m_keyFilter = filter;

        emit keyFilterChanged();
    }
}

void WebOSCoreCompositor::handleActiveFocusItemChanged()
{
    if (m_keyFilter)
        m_keyFilter->keyFocusChanged();
}

void WebOSCoreCompositor::setAcquired(bool acquired)
{
    if (m_acquired == acquired)
        return;

    m_acquired = acquired;

    emit acquireChanged();
}

void WebOSCoreCompositor::notifyPointerEnteredSurface(QWaylandSurface *surface)
{
    qDebug() << "surface=" << surface;
}

void WebOSCoreCompositor::notifyPointerLeavedSurface(QWaylandSurface *surface)
{
    qDebug() << "surface=" << surface;
}

void WebOSCoreCompositor::directRenderingActivated(bool active)
{
    if (m_directRendering == active)
        return;

    m_directRendering = active;

    emit directRenderingChanged();
}

void WebOSCoreCompositor::setCursorVisible(bool visibility)
{
    if (m_cursorVisible != visibility) {
        m_cursorVisible = visibility;
        emit cursorVisibleChanged();
        static_cast<WebOSCompositorWindow *>(window())->setCursorVisible(visibility);
    }
}

void WebOSCoreCompositor::setSurfaceItemClosePolicy(QVariantMap &surfaceItemClosePolicy)
{
    if (!surfaceItemClosePolicy.isEmpty() && m_surfaceItemClosePolicy != surfaceItemClosePolicy) {
        m_surfaceItemClosePolicy = surfaceItemClosePolicy;
        emit surfaceItemClosePolicyChanged();
    }
}

/* Compositor should call this whenever system UI shows/disappears
 * to restore cursor shape without mouse move */
void WebOSCoreCompositor::updateCursorFocus()
{
    PMTRACE_FUNCTION;

#ifdef MULTIINPUT_SUPPORT
    static_cast<WebOSCompositorWindow *>(window())->updateCursorFocus((Qt::KeyboardModifiers)m_lastMouseEventFrom);
#else
    static_cast<WebOSCompositorWindow *>(window())->updateCursorFocus();
#endif
}

void WebOSCoreCompositor::setMouseEventEnabled(bool enable)
{
    if (m_mouseEventEnabled != enable) {
        m_mouseEventEnabled = enable;
        emit mouseEventEnabledChanged();
    }
}

#ifdef MULTIINPUT_SUPPORT
QWaylandInputDevice *WebOSCoreCompositor::queryInputDevice(QInputEvent *inputEvent)
{
    /* We will use defaultInputDevice for device id 0 */
    if (WebOSInputDevice::getDeviceId(inputEvent) == 0)
        return 0;

    WebOSInputDevice *newInputDevice = m_inputDevicePreallocated;
    newInputDevice->setDeviceId(inputEvent);
    m_inputDevicePreallocated = new WebOSInputDevice(this);

    return newInputDevice;
}
#endif

void WebOSCoreCompositor::emitLsmReady()
{
    PMTRACE_FUNCTION;
    QString upstartCmd = QLatin1String("/sbin/initctl emit --no-wait lsm-ready");
    qDebug("emit upstart '%s'", qPrintable(upstartCmd));
    QProcess::startDetached(upstartCmd);
}

int WebOSCoreCompositor::prepareOutputUpdate()
{
    foreach (WebOSSurfaceItem *item, m_surfaces) {
        if (!item->surface() || item->width() == item->height())
            continue;

        connect(item->surface(), &QWaylandSurface::sizeChanged, this, &WebOSCoreCompositor::onSurfaceSizeChanged);
        m_surfacesOnUpdate << item;
        qDebug() << "OutputGeometry: watching item for the size change -" << item;
    }

    return m_surfacesOnUpdate.count();
}

void WebOSCoreCompositor::commitOutputUpdate(QQuickWindow *window, QRect geometry, int rotation, double ratio)
{
    qInfo() << "OutputGeometry: sending output update to clients:" << window << geometry << rotation << ratio;

    QWaylandOutput *output = m_outputs.value(window);

    if (!output) {
        qWarning() << "Could not update output as no output found for given window" << window;
        return;
    }

    output->setGeometry(QRect(QPoint(), geometry.size() * ratio));

    if (rotation % 360 == 270)
        setScreenOrientation(output, Qt::PortraitOrientation);
    else if (rotation % 360 == 90)
        setScreenOrientation(output, Qt::InvertedPortraitOrientation);
    else if (rotation % 360 == 180)
        setScreenOrientation(output, Qt::InvertedLandscapeOrientation);
    else
        setScreenOrientation(output, Qt::LandscapeOrientation);
}

void WebOSCoreCompositor::finalizeOutputUpdate()
{
    foreach (WebOSSurfaceItem *item, m_surfacesOnUpdate) {
        if (item->surface())
            disconnect(item->surface(), &QWaylandSurface::sizeChanged, this, &WebOSCoreCompositor::onSurfaceSizeChanged);
    }
    m_surfacesOnUpdate.clear();
}

void WebOSCoreCompositor::onSurfaceSizeChanged()
{
    QWaylandQuickSurface *surface = qobject_cast<QWaylandQuickSurface *>(sender());
    WebOSSurfaceItem* item = qobject_cast<WebOSSurfaceItem*>(surface->surfaceItem());

    qDebug() << "OutputGeometry: size changed for item -" << item;

    m_surfacesOnUpdate.removeOne(item);
    disconnect(surface, &QWaylandSurface::sizeChanged, this, &WebOSCoreCompositor::onSurfaceSizeChanged);

    // We assume that if size is updated, output changes are applied to client.
    if (m_surfacesOnUpdate.isEmpty())
        emit outputUpdateDone();
}

void WebOSCoreCompositor::initializeExtensions(WebOSCoreCompositor::ExtensionFlags extensions)
{
    if (extensions == WebOSCoreCompositor::NoExtensions) {
        return;
    }

    if (extensions & WebOSCoreCompositor::SurfaceGroupExtension) {
        m_surfaceGroupCompositor = new WebOSSurfaceGroupCompositor(this);
    }
}

void WebOSCoreCompositor::initTestPluginLoader()
{
    CompositorExtensionFactory::watchTestPluginDir();
}

QList<QWaylandInputDevice *> WebOSCoreCompositor::inputDevices() const
{
    return handle()->inputDevices();
}

QWaylandInputDevice *WebOSCoreCompositor::inputDeviceFor(QInputEvent *inputEvent)
{
#ifdef MULTIINPUT_SUPPORT
    QWaylandInputDevice *dev = NULL;
    // The last input device in the input device list must be default input device
    // which is QWaylandInputDevice, so that it always returns true for isOwner().
    for (int i = 0; i < inputDevices().size() - 1; i++) {
        QWaylandInputDevice *candidate = inputDevices().at(i);
        if (candidate->isOwner(inputEvent)) {
            dev = candidate;
            return dev;
        }
    }

    dev =  queryInputDevice(inputEvent);
    if (!dev) {
        dev = defaultInputDevice();
    }

    return dev;
#else
    return QWaylandCompositor::inputDeviceFor(inputEvent);
#endif
}

WebOSCoreCompositor::EventPreprocessor::EventPreprocessor(WebOSCoreCompositor* compositor)
    : QObject()
    , m_compositor(compositor)
{
}

bool WebOSCoreCompositor::EventPreprocessor::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    bool eventAccepted = false;

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);

#ifdef MULTIINPUT_SUPPORT
        // Make sure input device ready before synchronizing modifier state.
        m_compositor->inputDeviceFor(ke);

        // Update key modifier state for all input devices
        // so that they're always in sync with lock state.
        foreach (QWaylandInputDevice *dev, m_compositor->inputDevices()) {
            dev->updateModifierState(ke);
        }
#else
        m_compositor->defaultInputDevice()->updateModifierState(ke);
#endif
    }

#ifdef MULTIINPUT_SUPPORT
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouse = static_cast<QMouseEvent *>(event);
        m_compositor->m_lastMouseEventFrom = WebOSInputDevice::getDeviceId(mouse);
    }
#endif

    return eventAccepted;
}
