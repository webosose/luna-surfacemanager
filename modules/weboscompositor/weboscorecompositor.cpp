// Copyright (c) 2014-2022 LG Electronics, Inc.
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

#include <qwaylandquickitem.h>
#include <QWaylandSeat>
#include <QtGui/qpa/qwindowsysteminterface_p.h>
#include <QtWaylandCompositor/private/qwaylandkeyboard_p.h>
#include <QtWaylandCompositor/private/qwaylandseat_p.h>
#include <QtWaylandCompositor/private/qwaylandcompositor_p.h>
#include <QtWaylandCompositor/qwaylandqtwindowmanager.h>

#include <QDebug>
#include <QQuickWindow>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QQmlComponent>
#include <QProcess>
#include <QScreen>
#include <QJsonArray>
#include <QWaylandQuickOutput>

#include "weboscorecompositor.h"
#include "weboscompositorwindow.h"
#include "webosforeign.h"
#include "weboswindowmodel.h"
#include "webosgroupedwindowmodel.h"
#include "webossurfacemodel.h"
#include "webossurface.h"
#include "webossurfaceitem.h"
#include "webossurfaceitemmirror.h"
#include "webossurfacegroup.h"
#include "webosshellsurface.h"
#include "webosinputdevice.h"
#include "webosseat.h"
#include "webosinputmethod.h"
#include "webosevent.h"
#include "compositorextension.h"
#include "compositorextensionfactory.h"

#include "webossurfacegroupcompositor.h"
#include "webosscreenshot.h"

// Needed extra for type registration
#include "weboskeyfilter.h"
#include "webosinputmethod.h"
#include "waylandinputmethod.h"
#include "waylandinputmethodmanager.h"

#include "weboskeyboard.h"
#include "webostablet/webostablet.h"

#include "weboscompositortracer.h"
#include "weboscompositorconfig.h"

#include "weboswaylandseat.h"

// Need to access QtWayland::Keyboard::focusChanged
#include <QtWaylandCompositor/private/qwaylandsurface_p.h>

#ifdef USE_PMLOGLIB
#include <PmLogLib.h>
#endif

class WebOSCoreCompositorPrivate : public QWaylandCompositorPrivate
{
public:
    WebOSCoreCompositorPrivate(WebOSCoreCompositor *compositor)
        : QWaylandCompositorPrivate(compositor)
    {};

    QWaylandSeat *seatFor(QInputEvent *inputEvent) override;

    Q_DECLARE_PUBLIC(WebOSCoreCompositor)
};

QWaylandSeat *WebOSCoreCompositorPrivate::seatFor(QInputEvent *inputEvent)
{
    Q_Q(WebOSCoreCompositor);
#ifdef MULTIINPUT_SUPPORT
    QWaylandSeat *dev = NULL;
    // The first input device in the input device list must be default input device
    // which is QWaylandSeat, so that it always returns true for isOwner().
    for (int i = 1; i < seats.size(); i++) {
        QWaylandSeat *candidate = seats.at(i);
        if (candidate->isOwner(inputEvent)) {
            dev = candidate;
            return dev;
        }
    }

    dev = q->queryInputDevice(inputEvent);
    if (!dev)
        dev = q->defaultSeat();

    return dev;
#else
    QEvent::Type type = inputEvent->type();
    if (type == QEvent::TouchBegin || type == QEvent::TouchUpdate ||
        type == QEvent::TouchEnd || type == QEvent::TouchCancel) {
        QTouchEvent *touch = static_cast<QTouchEvent *>(inputEvent);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        if (touch->window())
            return static_cast<WebOSCompositorWindow *>(touch->window())->inputDevice();
#endif
    }

    if (type == QEvent::MouseMove || type == QEvent::MouseButtonPress ||
        type ==  QEvent::MouseButtonRelease) {
        WebOSMouseEvent *wMouse = nullptr;
        QMouseEvent *mouse = static_cast<QMouseEvent *>(inputEvent);
        if (mouse->source() == Qt::MouseEventSource::MouseEventSynthesizedByApplication)
            wMouse = static_cast<WebOSMouseEvent *>(mouse);
        if (wMouse && wMouse->window())
            return static_cast<WebOSCompositorWindow *>(wMouse->window())->inputDevice();
    }

    if (type == QEvent::Wheel) {
        WebOSWheelEvent *wWheel = nullptr;
        QWheelEvent *wheel = static_cast<QWheelEvent *>(inputEvent);
        if (wheel->source() == Qt::MouseEventSource::MouseEventSynthesizedByApplication)
            wWheel = static_cast<WebOSWheelEvent *>(wheel);
        if (wWheel && wWheel->window())
            return static_cast<WebOSCompositorWindow *>(wWheel->window())->inputDevice();
    }

    if (type == QEvent::HoverEnter || type == QEvent::HoverLeave) {
        QHoverEvent *hover = static_cast<QHoverEvent *>(inputEvent);
        if (hover) {
            WebOSHoverEvent *wHover = dynamic_cast<WebOSHoverEvent *>(hover);
            if (wHover && wHover->window())
                return static_cast<WebOSCompositorWindow *>(wHover->window())->inputDevice();
        }
    }

    return QWaylandCompositorPrivate::seatFor(inputEvent);
#endif
}

void WebOSCoreCompositor::logger(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const char* function = context.function;
    QByteArray userMessageUtf8 = message.toUtf8();
    const char* userMessage = userMessageUtf8.constData();

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
    : QWaylandCompositor(*new WebOSCoreCompositorPrivate(this))
    , m_previousFullscreenSurfaceItem(0)
    , m_fullscreenSurfaceItem(nullptr)
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
    , m_inputManager(0)
    , m_surfaceModel(nullptr)
    , m_wlShell(new QWaylandWlShell(this))
#ifdef MULTIINPUT_SUPPORT
    , m_lastMouseEventFrom(0)
    , m_inputDevicePreallocated(0)
#endif
    , m_autoStart(true)
    , m_loaded(false)
    , m_respawned(false)
    , m_registered(false)
    , m_extensionFlags(extensions)
{
    setSocketName(socketName);

    connect(this, &QWaylandCompositor::surfaceRequested, this, [this] (QWaylandClient *client, uint id, int version) {
        WebOSSurface *surface = new WebOSSurface();
        surface->initialize(this, client, id, version);
    });
    connect(this, &QWaylandCompositor::surfaceCreated, this, &WebOSCoreCompositor::surfaceCreated);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    const QVector<ShmFormat> supportedWaylandFormats = {
        ShmFormat_ARGB8888,
        ShmFormat_XRGB8888,
        ShmFormat_C8,
        ShmFormat_XRGB4444,
        ShmFormat_ARGB4444,
        ShmFormat_XRGB1555,
        ShmFormat_RGB565,
        ShmFormat_RGB888,
        ShmFormat_XBGR8888,
        ShmFormat_ABGR8888,
        ShmFormat_XRGB2101010,
        ShmFormat_XBGR2101010,
        ShmFormat_ARGB2101010,
        ShmFormat_ABGR2101010
    };

    QWaylandCompositor::setAdditionalShmFormats(supportedWaylandFormats);

    // Remove any WindowSystemEventHandler installed by the base class.
    if (QWindowSystemInterfacePrivate::eventHandler)
        QWindowSystemInterfacePrivate::removeWindowSystemEventhandler(QWindowSystemInterfacePrivate::eventHandler);
#endif
}

WebOSCoreCompositor::~WebOSCoreCompositor()
{
    QCoreApplication::instance()->removeEventFilter(m_eventPreprocessor);
    delete m_eventPreprocessor;
    delete m_wlShell;
}

void WebOSCoreCompositor::insertToWindows(WebOSCompositorWindow *window)
{
    if (!window || window->displayId() < 0) {
        qWarning() << "Cannot insert window to list";
        return;
    }

    int displayId = window->displayId();
    int sizeNeeded = displayId + 1;

    // The order of the insertion is not a concern,
    // but all slots should be inserted after all.
    if (m_windows.size() < sizeNeeded)
        m_windows.resize(sizeNeeded);

    m_windows[displayId] = window;
    updateWindowPositionInCluster();
    emit windowsChanged();
}

void WebOSCoreCompositor::create()
{
    initializeExtensions(m_extensionFlags);
    QWaylandCompositor::create();
    checkDaemonFiles();
}

void WebOSCoreCompositor::registerWindow(QQuickWindow *window, QString name)
{
    QWaylandQuickOutput *output = new QWaylandQuickOutput(this, window);
    if (!output) {
        qCritical() << "Failed to create QWaylandOutput for window" << window << name;
        return;
    }

    QWaylandOutputMode mode(m_outputGeometry.isValid() ? m_outputGeometry.size() : window->size(), 60000);
    output->addMode(mode, true);
    output->setCurrentMode(mode);

    WebOSCompositorWindow *webosWindow = static_cast<WebOSCompositorWindow *>(window);
    insertToWindows(webosWindow);
    webosWindow->setOutput(output);
    output->setModel(webosWindow->modelString());

    qInfo() << "Registering a compositor window" << webosWindow << webosWindow->displayId() << webosWindow->screen()->name() << webosWindow->modelString();

    //TODO: check is it ok just to use primary window to handle activeFocusItem
    connect(window, &QQuickWindow::activeFocusItemChanged, this, &WebOSCoreCompositor::handleActiveFocusItemChanged);

    if (!m_registered) {
        m_registered = true;

        setDefaultOutput(output);

        m_surfaceModel = new WebOSSurfaceModel();

        setInputMethod(createInputMethod());
        m_inputMethod->initialize();

        connect(m_unixSignalHandler, &UnixSignalHandler::sighup, this, &WebOSCoreCompositor::reloadConfig);
        connect(m_unixSignalHandler, &UnixSignalHandler::sighup, WebOSCompositorConfig::instance(), &WebOSCompositorConfig::dump);

        // TODO: support multiple keyboard focus
        connect(defaultSeat()->keyboard(), &QWaylandKeyboard::focusChanged, this, &WebOSCoreCompositor::activeSurfaceChanged);

        QCoreApplication::instance()->installEventFilter(m_eventPreprocessor);

        m_extensions = CompositorExtensionFactory::create(this);

        m_shell = new WebOSShell(this);

        m_inputManager = new WebOSInputManager(this);
#ifdef MULTIINPUT_SUPPORT
        m_inputDevicePreallocated = new WebOSInputDevice(this);
#endif
        m_webosTablet.reset(new WebOSTablet(this));
        // Set default state of Qt client windows to fullscreen
        QWaylandQtWindowManager *wmExtension = QWaylandQtWindowManager::findIn(this);
        if (wmExtension != nullptr)
            wmExtension->setShowIsFullScreen(true);

        if (m_foreign)
            m_foreign->registeredWindow();

        emit surfaceModelChanged();
        emit windowChanged();

        // Use defaultInputDevice for primary window
        webosWindow->setInputDevice(defaultSeat());
    } else {
        // Create dedicated InputDevice for secondary windows
        // Pointer is for mouseFocus which is needed for touch
        QWaylandSeat *device =
            new WebOSInputDevice(this, QWaylandSeat::Keyboard | QWaylandSeat::Touch | QWaylandSeat::Pointer);
        webosWindow->setInputDevice(device);

        // Install the key filter for secondary windows.
        if (m_keyFilter)
            webosWindow->installEventFilter(m_keyFilter);
    }
}

void WebOSCoreCompositor::checkDaemonFiles()
{
    QByteArray xdgDir = qgetenv("XDG_RUNTIME_DIR");

    QByteArray name(socketName());
    // Similar logic as in wl_display_add_socket()
    if (name.isEmpty()) {
        name = qgetenv("WAYLAND_DISPLAY");
        if (name.isEmpty())
            name = "wayland-0";

        setSocketName(name);
    }

    QFileInfo sInfo(QString("%1/%2").arg(xdgDir.constData()).arg(name.data()));
    QFileInfo dInfo(sInfo.absoluteDir().absolutePath());
    if (QFile::setPermissions(sInfo.absoluteFilePath(),
                QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                QFileDevice::ReadGroup | QFileDevice::WriteGroup)) {
        // TODO: Qt doesn't provide a method to chown at the moment
        qDebug() << "Setting ownership of" << sInfo.absoluteFilePath() << "using /bin/chown";
        QProcess::startDetached("/bin/chown", { QString("%1:%2").arg(dInfo.owner()).arg(dInfo.group()), sInfo.absoluteFilePath() }, "./");
    } else {
        qCritical() << "Unable to set permission for" << sInfo.absoluteFilePath();
    }

    QFile pInfo(QString("%1/surface-manager.pid").arg(xdgDir.constData()));
    if (pInfo.exists()) {
        qInfo() << "surface-manager instance was respawned for some reason";
        m_respawned = true;
        emit respawnedChanged();
    }
    if (pInfo.open(QIODevice::WriteOnly | QFile::Truncate)) {
        QTextStream out(&pInfo);
        out << QCoreApplication::applicationPid() << '\n';
        pInfo.close();
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
    qmlRegisterType<WaylandInputMethod>("WebOSCoreCompositor", 1, 0, "WaylandInputMethod");
    qmlRegisterType<WebOSSurfaceGroup>("WebOSCoreCompositor", 1, 0, "SurfaceItemGroup");
    qmlRegisterType<WebOSScreenShot>("WebOSCoreCompositor", 1, 0, "ScreenShot");
    qmlRegisterUncreatableType<WebOSKeyPolicy>("WebOSCoreCompositor", 1, 0, "KeyPolicy", QLatin1String("Not allowed to create KeyPolicy instance"));
    qmlRegisterUncreatableType<WebOSCompositorWindow>("WebOSCoreCompositor", 1, 0, "CompositorWindow", QLatin1String("Not allowed to create CompositorWindow"));
    qmlRegisterType<WebOSSurfaceItemMirror>("WebOSCoreCompositor", 1, 0, "SurfaceItemMirror");
}

QWaylandQuickSurface* WebOSCoreCompositor::fullscreenSurface() const
{
    return m_fullscreenSurfaceItem ? qobject_cast<QWaylandQuickSurface *>(m_fullscreenSurfaceItem->surface()) : nullptr;
}

WebOSSurfaceItem* WebOSCoreCompositor::fullscreenSurfaceItem() const
{
    return m_fullscreenSurfaceItem;
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

        connect(m_inputMethod, &WaylandInputMethod::activeChanged, [this] {
            if (m_inputMethod) {
                if (m_inputMethod->active())
                    emit this->inputPanelRequested();
                else
                    emit this->inputPanelDismissed();
            }
        });

        emit inputMethodChanged();
    }
}

bool WebOSCoreCompositor::isMapped(WebOSSurfaceItem *item)
{
    if (!item->surface() || !item->surface()->hasContent())
        return false;

    if (webOSWindowExtension())
        return m_surfaceModel->indexFromItem(item).isValid();
    else
        return m_surfaces.contains(item);
}

/*
 * update our internal model of mapped surface in response to wayland surfaces being mapped
 * and unmapped. WindowModels use this as their source of windows.
 */
void WebOSCoreCompositor::onSurfaceMapped(QWaylandSurface *surface, WebOSSurfaceItem* item) {
    PMTRACE_FUNCTION;

    if (item && !item->imported()) {
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
    QWaylandSurface* active = defaultSeat()->keyboardFocus();
    if (!active)
        return 0;

    Q_ASSERT(active->views().first());
    QWaylandQuickItem *item = static_cast<QWaylandQuickItem*>(active->views().first()->renderObject());
    return qobject_cast<WebOSSurfaceItem*>(item);
}

QList<QObject *> WebOSCoreCompositor::windows() const
{
    QList<QObject *> windowObjects;

    for (int i = 0; i < m_windows.size(); ++i)
        windowObjects << m_windows[i];

    return windowObjects;
}

WebOSCompositorWindow *WebOSCoreCompositor::window(int displayId)
{
    if (m_windows.size() <= displayId)
        return nullptr;

    return m_windows[displayId];
}

void WebOSCoreCompositor::updateWindowPositionInCluster()
{
    m_clusters.clear();

    QJsonObject displayCluster = WebOSCompositorConfig::instance()->displayCluster().object();
    for (const auto &cluster: displayCluster.keys()) {
        QPoint positionInCluster;
        QJsonArray row = displayCluster[cluster].toArray();
        for (int r = 0; r < row.size(); r++) {
            int maxHeight = 0;
            positionInCluster.setX(0);
            QJsonArray col = row[r].toArray();
            for (int c = 0; c < col.size(); c++) {
                foreach (WebOSCompositorWindow *w, m_windows) {
                    if (w->displayName() == col[c].toString()) {
                        w->setPositionInCluster(positionInCluster);
                        positionInCluster.rx() += w->outputGeometry().width();
                        maxHeight = qMax(maxHeight, w->outputGeometry().height());

                        // Update cluster size
                        if (m_clusters[cluster].indexOf(w) == -1)
                            m_clusters[cluster] << w;
                        int clusterWidth = qMax(w->clusterSize().width(), w->positionInCluster().x() + w->outputGeometry().width());
                        int clusterHeight = qMax(w->clusterSize().height(), w->positionInCluster().y() + w->outputGeometry().height());
                        for (auto &cw : m_clusters[cluster]) {
                            cw->setClusterName(cluster);
                            cw->setClusterSize(QSize(clusterWidth, clusterHeight));
                        }
                        break;
                    }
                }
            }
            positionInCluster.ry() += maxHeight;
        }
    }

    qDebug() << "Display cluster configuration:" << m_clusters;
}

QList<QObject *> WebOSCoreCompositor::windowsInCluster(QString clusterName)
{
    QList<QObject *> windows;
    if (clusterName.isEmpty() || m_clusters.size() == 0 || !m_clusters.contains(clusterName))
        return windows;

    for (const auto &w: m_clusters[clusterName])
        windows << w;

    return windows;
}

void WebOSCoreCompositor::onSurfaceUnmapped(QWaylandSurface *surface, WebOSSurfaceItem* item) {
    PMTRACE_FUNCTION;

    if (!item) {
        qWarning() << "No item for surface" << surface;
        return;
    }

    qInfo() << surface << item << item->appId() << item->itemState();

    if (webOSWindowExtension()) {
        if (item->appId().isEmpty()) {
            m_surfaceModel->surfaceUnmapped(item);
            emit surfaceUnmapped(item);
            m_surfaces.removeOne(item);
            m_surfacesOnUpdate.removeOne(item);
        } else {
            processSurfaceItem(item, WebOSSurfaceItem::ItemStateHidden);
        }
    } else {
        if (item->itemState() != WebOSSurfaceItem::ItemStateProxy) {
            m_surfaceModel->surfaceUnmapped(item);
            emit surfaceUnmapped(item);
            m_surfaces.removeOne(item);
            m_surfacesOnUpdate.removeOne(item);
        } else {
            emit surfaceUnmapped(item); //We have to notify qml even for proxy item
        }
    }
}

void WebOSCoreCompositor::onSurfaceDestroyed(QWaylandSurface *surface, WebOSSurfaceItem* item) {
    PMTRACE_FUNCTION;

    if (!item) {
        qWarning() << "No item for surface" << surface;
        return;
    }

    qInfo() << surface << item << item->appId() << item->itemState() << item->itemStateReason();

    if (webOSWindowExtension()) {
        if (item->appId().isEmpty()) {
            removeSurfaceItem(item, true);
        } else {
            processSurfaceItem(item, WebOSSurfaceItem::ItemStateProxy);
        }
    } else {
        if (item->itemState() != WebOSSurfaceItem::ItemStateProxy) {
            removeSurfaceItem(item, true);
        } else {
            emit surfaceDestroyed(item); //We have to notify qml even for proxy item
            // This means items will not use any graphic resource from related surface.
            // If there are more use case, the API should be moved to proper place.
            // ex)If there are some dying animation for the item, this should be called at the end of the animation.
            item->view()->setSurface(nullptr);
        }

        if (item == m_fullscreenSurfaceItem)
            setFullscreen(nullptr);
    }
}

/* Basic life cycle of surface and surface item.
   1. new QtWayland::Surface { new QWaylandSurface }
        -> QWebOSCoreCompositor::surfaceCreated { new QWebOSSurfaceItem }

   2. delete QtWayland::Surface { delete QWaylandSurface {
        ~QObject() {
         emit destroyed(QObject *) -> WebOSCoreCompositor::surfaceDestroyed() / QWaylandQuickItem::surfaceDestroyed()
         deleteChilderen -> ~QWaylandSurfacePrivate()
        }
      }}

   We will delete QWebOSSurfaceItem in WebOSCoreCompositor::surfaceDestroyed as a pair of surfaceCreated.
   For that, we have to guarantee WebOSCoreCompositor::surfaceDestroyed is called before QWaylandQuickItem::surfaceDestroyed.
   Otherwise, the surface item will lose a reference for surface in QWaylandQuickItem::surfaceDestroyed.
   After all, ~QWebOSSurfaceItem cannot clear surface's reference, then the surface still have invaild reference to
   surface item which is already freed. */

void WebOSCoreCompositor::surfaceCreated(QWaylandSurface *surface) {
    PMTRACE_FUNCTION;

    /* Ensure that WebOSSurfaceItem is created after surfaceDestroyed is connected.
       Refer to upper comment about life cycle of surface and surface item. */
    WebOSSurfaceItem *item = createSurfaceItem(static_cast<QWaylandQuickSurface *>(surface));

    connect(surface, &QWaylandSurface::hasContentChanged, [this, surface, item] {
        if (surface->hasContent()) {
            this->onSurfaceMapped(surface, item);
        } else if (item->surface() && !item->isBufferLocked()) {
            // If the buffer is locked, surfaceUnmapped for that item should be
            // handled later once the buffer gets unlocked (by handleSurfaceUnmapped)
            this->onSurfaceUnmapped(surface, item);
        }
    });

    connect(surface, &QWaylandSurface::destroyed, [this, surface, item] {
        this->onSurfaceDestroyed(surface, item);
    });

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

void WebOSCoreCompositor::addSurfaceItem(WebOSSurfaceItem *item)
{
    m_surfaceModel->surfaceMapped(item);
    m_surfaces << item;
    emit surfaceMapped(item);
}

void WebOSCoreCompositor::removeSurfaceItem(WebOSSurfaceItem* item, bool emitSurfaceDestroyed)
{
    PMTRACE_FUNCTION;

    qInfo() << "removing item" << item << item->itemState() << item->itemStateReason();
    m_surfaceModel->surfaceDestroyed(item);
    if (emitSurfaceDestroyed)
        emit surfaceDestroyed(item);
    m_surfaces.removeOne(item);
    m_surfacesOnUpdate.removeOne(item);
    delete item;
}

void WebOSCoreCompositor::handleSurfaceUnmapped(WebOSSurfaceItem* item)
{
    qInfo() << "handling surfaceUnmapped for" << item << "upon request";
    if (item)
        onSurfaceUnmapped(item->surface(), item);
}

WebOSSurfaceItem* WebOSCoreCompositor::fullscreen() const
{
    return m_fullscreenSurfaceItem;
}

void WebOSCoreCompositor::setFullscreen(WebOSSurfaceItem* item)
{
    PMTRACE_FUNCTION;
    QWaylandQuickSurface* surface = item ? qobject_cast<QWaylandQuickSurface *>(item->surface()) : nullptr;
    // NOTE: Some surface is not QWaylandQuickSurface (e.g. Cursor), we still need more attention in that case.
    // TODO Move the block to qml
    if (!surface) {
        emit homeScreenExposed();
    }

    if (item != m_fullscreenSurfaceItem) {
        m_inputMethod->deactivate();
        // The notion of the fullscreen surface needs to remain here for now as
        // direct rendering support needs a handle to it
        if (item == NULL) {
            m_previousFullscreenSurfaceItem = NULL;
        } else {
            m_previousFullscreenSurfaceItem = m_fullscreenSurfaceItem;
        }
        m_fullscreenSurfaceItem = item;
        emit fullscreenChanged();
        emit fullscreenSurfaceChanged();
        // This is here for a transitional period, see signal comments
        QWaylandSurface *previousFullscreenSurface = m_previousFullscreenSurfaceItem ? m_previousFullscreenSurfaceItem->surface() : nullptr;
        emit fullscreenSurfaceChanged(previousFullscreenSurface, surface);
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

    qDebug() << item << item->itemState() << stateToBe << item->itemStateReason() << item->isSurfaced();

    // Possible state transitions
    // 1) ItemStateNormal
    //    This is the default state when an item is created. No transitions allowed to this from others.
    // 2) ItemStateHidden
    //    Indicates the item has not a visible surface. Only an item in ItemStateNormal can transition
    //    to this state by a surfaceUnmapped signal.
    // 3) ItemStateProxy
    //    Indicates the surface of the item has been destroyed. An item in ItemStateNormal or Hidden
    //    can transition to this state by a surfaceDestroyed signal.
    // 4) ItemStateClosing
    //    Indicates the item has been set for its state reason and is about to be destroyed.
    //    Note that the corresponding surface may exist and be even in visible state while the item is
    //    in this state. An item in this state should also be handled properly when it received a
    //    surfaceUnmapped or surfaceDestroyed signal.
    switch (stateToBe) {
    case WebOSSurfaceItem::ItemStateNormal:
        qWarning() << "no transition to ItemStateNormal for" << item << item->itemState() << item->itemStateReason();
        break;
    case WebOSSurfaceItem::ItemStateHidden:
        if (!item->isSurfaced() || !item->surface()->hasContent()) {
            switch (item->itemState()) {
            case WebOSSurfaceItem::ItemStateNormal:
                qInfo() << "transitioning to ItemStateHidden for" << item << item->itemState() << item->itemStateReason();
                item->setItemState(WebOSSurfaceItem::ItemStateHidden, item->itemStateReason());
                // Fall through
            case WebOSSurfaceItem::ItemStateClosing:
            case WebOSSurfaceItem::ItemStateHidden:
                if(item->itemStateReason().isEmpty()) {
                    qInfo() << "unhandled case of surfaceUnmapped for " << item << item->itemState() << item->itemStateReason();
                    break;
                }
                qInfo() << "handling surfaceUnmapped for " << item << item->itemState() << item->itemStateReason();
                emit surfaceUnmapped(item);
                // reset state reason to re-use
                item->unsetItemStateReason();
                break;
            default:
                qWarning() << "unhandled case of a transition to ItemStateHidden for" << item << item->itemState() << item->itemStateReason();
                break;

            }
        } else {
            qWarning() << "not ready to be ItemStateHidden," << item << item->itemState() << item->itemStateReason();
        }
        break;
    case WebOSSurfaceItem::ItemStateProxy:
        if (!item->isSurfaced() || !item->surface()->hasContent()) {
            switch (item->itemState()) {
            case WebOSSurfaceItem::ItemStateNormal:
            case WebOSSurfaceItem::ItemStateHidden:
                if (!item->itemStateReason().isEmpty() && checkSurfaceItemClosePolicy(item->itemStateReason(), item)) {
                    qInfo() << "transitioning to ItemStateProxy and ItemStateClosing for" << item << item->itemState() << item->itemStateReason();
                    item->setItemState(WebOSSurfaceItem::ItemStateProxy, item->itemStateReason());
                    processSurfaceItem(item, WebOSSurfaceItem::ItemStateClosing);
                } else {
                    qInfo() << "transitioning to ItemStateProxy for" << item << item->itemState() << item->itemStateReason();
                    item->setItemState(WebOSSurfaceItem::ItemStateProxy, item->itemStateReason());
                    emit surfaceDestroyed(item);
                    // reset state reason to re-use
                    item->unsetItemStateReason();

                    // Extra clean-up for surface group items
                    if (item->isSurfaceGroupRoot()) {
                        qInfo() << "closing surface group member items for" << this;
                        item->sendCloseToGroupItems();
                    } else if (item->isPartOfGroup()) {
                        qInfo() << "closing a surface group member item:" << item;
                        item->setItemState(WebOSSurfaceItem::ItemStateClosing, item->itemStateReason());
                    }

                    /* This means items will not use any graphic resource from related surface.
                    / If there are more use case, the API should be moved to proper place.
                    / ex)If there are some dying animation for the item, this should be called at the end of the animation. */
                    item->view()->setSurface(nullptr);
                    // Clear old texture
                    item->update();
                }
                break;
            case WebOSSurfaceItem::ItemStateClosing:
                qInfo() << "handling surfaceDestroyed for " << item << item->itemState() << item->itemStateReason();
                // remove item
                removeSurfaceItem(item, true);
                break;
            default:
                qWarning() << "unhandled case of the transition to ItemStateProxy for" << item << item->itemState() << item->itemStateReason();
                break;
            }
        } else {
            qWarning() << "not ready to be ItemStateProxy," << item << item->itemState() << item->itemStateReason();
        }
        break;
    case WebOSSurfaceItem::ItemStateClosing:
        qInfo() << "transitioning to ItemStateClosing for" << item << item->itemState() << item->itemStateReason();
        item->setItemState(WebOSSurfaceItem::ItemStateClosing, item->itemStateReason());
        if (!item->isSurfaced() || !item->surface()->hasContent()) {
            // remove item
            removeSurfaceItem(item, true);
        } else {
            qWarning() << "item is not yet ready to be removed," << item << item->itemState() << item->itemStateReason();
        }
        break;
    default:
        break;
    }
}

void WebOSCoreCompositor::destroyClientForWindow(QVariant window) {
    QWaylandSurface *surface = qobject_cast<QWaylandQuickItem *>(qvariant_cast<QObject *>(window))->surface();
    destroyClientForSurface(surface);
}

bool WebOSCoreCompositor::getCursor(QWaylandSurface *surface, int hotSpotX, int hotSpotY, QCursor& cursor)
{
    Q_UNUSED(surface);
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

void WebOSCoreCompositor::setCursorSurface(QWaylandSurface *surface, int hotspotX, int hotspotY, wl_client *client)
{
    PMTRACE_FUNCTION;

    if (surface) {
        auto view = surface->primaryView();
        auto qs = static_cast<WebOSSurfaceItem*>(view->renderObject());
        if (qs)
            surface->disconnect(SIGNAL(hasContentChanged()));
    }

    foreach(WebOSSurfaceItem *item, m_surfaces) {
        if (item->surface() && !item->surface()->isCursorSurface() &&
                item->surface()->client() && item->surface()->client()->client() == client)
            item->setCursorSurface(surface, hotspotX, hotspotY);
    }
}

/* If HomeScreen is visible App cannot get any events.
 * So send pointer leave/enter event to fullscreen surface. */
void WebOSCoreCompositor::setMouseFocus(QWaylandSurface* surface)
{
    PMTRACE_FUNCTION;
    if (!surface) {
        foreach (WebOSCompositorWindow *w, m_windows)
            static_cast<WebOSCompositorWindow *>(w)->setDefaultCursor();
        return;
    }

#ifdef MULTIINPUT_SUPPORT
    foreach (QWaylandSeat *dev, inputDevices()) {
        if (surface && !surface->views().isEmpty())
            dev->setMouseFocus(surface->views().first());
    }
#else
    QWaylandQuickSurface *qsurface = static_cast<QWaylandQuickSurface *>(surface);
    WebOSSurfaceItem *item = WebOSSurfaceItem::getSurfaceItemFromSurface(qsurface);
    defaultSeat()->setMouseFocus(item ? item->view(): nullptr);
#endif
}

#ifdef MULTIINPUT_SUPPORT
void WebOSCoreCompositor::resetMouseFocus(QWaylandSurface *surface)
{
    QPointF curPosition = static_cast<QPointF>(QCursor::pos());
    foreach (QWaylandSeat *dev, inputDevices()) {
        if (surface && !surface->views().isEmpty()
            && dev->mouseFocus() == surface->views().first())
            dev->setMouseFocus(NULL);
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
        for (int i = 0; i < m_windows.size(); ++i) {
            m_windows[i]->removeEventFilter(m_keyFilter);
            m_windows[i]->installEventFilter(filter);
        }

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

const QList<WebOSSurfaceItem *> WebOSCoreCompositor::getItems(WebOSCompositorWindow *window) const
{
    if (!window)
        return m_surfaces;

    QList<WebOSSurfaceItem *> surfaces;
    foreach (WebOSSurfaceItem *item, m_surfaces) {
        if (item && item->window() == window)
            surfaces << item;
    }

    return surfaces;
}

void WebOSCoreCompositor::setCursorVisible(bool visibility)
{
    if (m_cursorVisible != visibility) {
        m_cursorVisible = visibility;
        emit cursorVisibleChanged();
        foreach (WebOSCompositorWindow *w, m_windows)
            static_cast<WebOSCompositorWindow *>(w)->setCursorVisible(visibility);
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

    foreach (WebOSCompositorWindow *w, m_windows) {
#ifdef MULTIINPUT_SUPPORT
        static_cast<WebOSCompositorWindow *>(w)->updateCursorFocus((Qt::KeyboardModifiers)m_lastMouseEventFrom);
#else
        static_cast<WebOSCompositorWindow *>(w)->updateCursorFocus();
#endif
    }
}

void WebOSCoreCompositor::setMouseEventEnabled(bool enable)
{
    if (m_mouseEventEnabled != enable) {
        m_mouseEventEnabled = enable;
        emit mouseEventEnabledChanged();
    }
}

#ifdef MULTIINPUT_SUPPORT
QWaylandSeat *WebOSCoreCompositor::queryInputDevice(QInputEvent *inputEvent)
{
    /* We will use defaultSeat for device id 0 */
    if (WebOSInputDevice::getDeviceId(inputEvent) == 0)
        return 0;

    WebOSInputDevice *newInputDevice = m_inputDevicePreallocated;
    newInputDevice->setDeviceId(inputEvent);
    m_inputDevicePreallocated = new WebOSInputDevice(this);

    return newInputDevice;
}
#endif

void WebOSCoreCompositor::setAutoStart(bool autoStart)
{
    if (m_autoStart != autoStart) {
        m_autoStart = autoStart;
        emit autoStartChanged();
    }
}

void WebOSCoreCompositor::emitLsmReady()
{
    PMTRACE_FUNCTION;

    qInfo("emitting loadCompleted and lsm-ready");

    m_loaded = true;
    emit loadCompleted();

    QProcess::startDetached("/sbin/initctl", { "emit", "--no-wait", "lsm-ready" }, "./");
}

int WebOSCoreCompositor::prepareOutputUpdate()
{
    foreach (WebOSSurfaceItem *item, m_surfaces) {
        if (!item->surface() || qFuzzyCompare(item->width(), item->height()) || item->state() == Qt::WindowMinimized)
            continue;

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        connect(item->surface(), &QWaylandSurface::bufferSizeChanged, this, &WebOSCoreCompositor::onSurfaceSizeChanged);
#else
        connect(item->surface(), &QWaylandSurface::sizeChanged, this, &WebOSCoreCompositor::onSurfaceSizeChanged);
#endif
        m_surfacesOnUpdate << item;
        qDebug() << "OutputGeometry: watching item for the size change -" << item;
    }

    return m_surfacesOnUpdate.count();
}

static void setScreenOrientation(QWaylandOutput *output, Qt::ScreenOrientation orientation) {
        bool isPortrait = output->window()->screen()->nativeOrientation() == Qt::PortraitOrientation;

        switch (orientation) {
        case Qt::PrimaryOrientation:
            output->setTransform(QWaylandOutput::TransformNormal);
            break;
        case Qt::LandscapeOrientation:
            output->setTransform(isPortrait ? QWaylandOutput::Transform270 : QWaylandOutput::TransformNormal);
            break;
        case Qt::PortraitOrientation:
            output->setTransform(isPortrait ? QWaylandOutput::TransformNormal : QWaylandOutput::Transform270);
            break;
        case Qt::InvertedLandscapeOrientation:
            output->setTransform(isPortrait ? QWaylandOutput::Transform90 : QWaylandOutput::Transform180);
            break;
        case Qt::InvertedPortraitOrientation:
            output->setTransform(isPortrait ? QWaylandOutput::Transform180 : QWaylandOutput::Transform90);
            break;
        }
}

void WebOSCoreCompositor::commitOutputUpdate(QQuickWindow *window, QRect geometry, int rotation, double ratio)
{
    qInfo() << "OutputGeometry: sending output update to clients:" << window << geometry << rotation << ratio;

    WebOSCompositorWindow *webosWindow = static_cast<WebOSCompositorWindow *>(window);
    QWaylandOutput *output = webosWindow->output();

    if (!output) {
        qWarning() << "Could not update output as no output found for given window" << window;
        return;
    }

    QWaylandOutputMode mode(geometry.size() * ratio, output->currentMode().refreshRate());
    output->addMode(mode, true);
    output->setCurrentMode(mode);

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
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
            disconnect(item->surface(), &QWaylandSurface::bufferSizeChanged, this, &WebOSCoreCompositor::onSurfaceSizeChanged);
#else
            disconnect(item->surface(), &QWaylandSurface::sizeChanged, this, &WebOSCoreCompositor::onSurfaceSizeChanged);
#endif
    }
    m_surfacesOnUpdate.clear();

    updateWindowPositionInCluster();
}

void WebOSCoreCompositor::onSurfaceSizeChanged()
{
    QWaylandQuickSurface *surface = qobject_cast<QWaylandQuickSurface *>(sender());
    WebOSSurfaceItem* item = WebOSSurfaceItem::getSurfaceItemFromSurface(surface);

    qDebug() << "OutputGeometry: size changed for item -" << item;

    m_surfacesOnUpdate.removeOne(item);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    disconnect(surface, &QWaylandSurface::bufferSizeChanged, this, &WebOSCoreCompositor::onSurfaceSizeChanged);
#else
    disconnect(surface, &QWaylandSurface::sizeChanged, this, &WebOSCoreCompositor::onSurfaceSizeChanged);
#endif

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

    if (extensions & WebOSCoreCompositor::WebOSForeignExtension)
        m_foreign.reset(createWebOSForeign());
}

void WebOSCoreCompositor::initTestPluginLoader()
{
    CompositorExtensionFactory::watchTestPluginDir();
}

QList<QWaylandSeat *> WebOSCoreCompositor::inputDevices() const
{
    Q_D(const WebOSCoreCompositor);
    return d->seats;
}

QWaylandSeat *WebOSCoreCompositor::keyboardDeviceForWindow(QQuickWindow *window)
{
    if (!window)
        return defaultSeat();

    return static_cast<WebOSCompositorWindow *>(window)->inputDevice();
}

QWaylandSeat *WebOSCoreCompositor::keyboardDeviceForDisplayId(int displayId)
{
    if (displayId < 0 || displayId >= m_windows.size()) {
        qWarning() << "Cannot get keyboard device for displayId" << displayId;
        return nullptr;
    }

    return m_windows[displayId]->inputDevice();
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
        m_compositor->seatFor(ke);

        // Update key modifier state for all input devices
        // so that they're always in sync with lock state.
        foreach (QWaylandSeat *dev, m_compositor->inputDevices()) {
            WebOSKeyboard *keyboard = qobject_cast<WebOSKeyboard *>(dev->keyboard());
            if (keyboard)
                keyboard->updateModifierState(ke->nativeScanCode(), (ke->type() == QEvent::KeyPress)? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED, ke->isAutoRepeat());
        }
#else
        WebOSKeyboard *keyboard = qobject_cast<WebOSKeyboard *>(m_compositor->defaultSeat()->keyboard());
        if (keyboard)
            keyboard->updateModifierState(ke->nativeScanCode(), (ke->type() == QEvent::KeyPress)? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED, ke->isAutoRepeat());
#endif
    }

#ifdef MULTIINPUT_SUPPORT
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouse = static_cast<QMouseEvent *>(event);
        m_compositor->m_lastMouseEventFrom = WebOSInputDevice::getDeviceId(mouse);
    }
#endif

    // These events are only sent to QGuiApp. So deliver these to all tablet client.
    if (event->type() == QEvent::TabletEnterProximity ||
        event->type() == QEvent::TabletLeaveProximity) {
        if (m_compositor->tabletDevice())
            m_compositor->tabletDevice()->advertiseApproximation((QTabletEvent*)event);
        eventAccepted = true;
    }

    return eventAccepted;
}

QWaylandSeat *WebOSCoreCompositor::createSeat()
{
    return new WebOSWaylandSeat(this);
}

QWaylandPointer *WebOSCoreCompositor::createPointerDevice(QWaylandSeat *seat)
{
    return QWaylandCompositor::createPointerDevice(seat);
}

QWaylandKeyboard *WebOSCoreCompositor::createKeyboardDevice(QWaylandSeat *seat)
{
    return new WebOSKeyboard(seat);
}

QWaylandTouch *WebOSCoreCompositor::createTouchDevice(QWaylandSeat *seat)
{
    return QWaylandCompositor::createTouchDevice(seat);
}

void WebOSCoreCompositor::registerSeat(QWaylandSeat *seat)
{
    Q_D(WebOSCoreCompositor);
    d->seats.append(seat);
}

void WebOSCoreCompositor::unregisterSeat(QWaylandSeat *seat)
{
    Q_D(WebOSCoreCompositor);
    d->seats.removeOne(seat);
}

WebOSSurfaceItem* WebOSCoreCompositor::createSurfaceItem(QWaylandQuickSurface *surface)
{
    return new WebOSSurfaceItem(this, surface);
}

WebOSInputMethod* WebOSCoreCompositor::createInputMethod()
{
    return new WebOSInputMethod(this);
}

WaylandInputMethodManager* WebOSCoreCompositor::createInputMethodManager(WaylandInputMethod *inputMethod)
{
    return new WaylandInputMethodManager(inputMethod);
}

WebOSForeign* WebOSCoreCompositor::createWebOSForeign()
{
    return new WebOSForeign(this);
}
