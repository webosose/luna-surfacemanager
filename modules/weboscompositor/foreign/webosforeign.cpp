// Copyright (c) 2018-2022 LG Electronics, Inc.
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

#include "videooutputd_communicator.h"
#include "punchthroughelement.h"
#include "weboscorecompositor.h"
#include "weboscompositorwindow.h"
#include "webosforeign.h"
#include "videowindow_informer.h"

#include <QDebug>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWaylandCompositor>

#include <qpa/qplatformnativeinterface.h>

static QString generateWindowId()
{
    static quint32 window_count = 0;
    return QString("_Window_Id_%1").arg(++window_count);
}

class MirrorItemHandler {

public:
    MirrorItemHandler() {}
    ~MirrorItemHandler() { clearHandler(); }

    static inline qreal getParentRatio(QQuickItem *dItem, QQuickItem *nItem, bool isWidth = true) {
        if (dItem && dItem->parentItem() && nItem && nItem->parentItem()) {
            if (isWidth)
                return dItem->parentItem()->width() / (qreal)nItem->parentItem()->width();

            return dItem->parentItem()->height() / (qreal)nItem->parentItem()->height();
        }

        return 1.0;
    }

    virtual void initialize(QQuickItem *item, QQuickItem *exportedItem, QQuickItem *source, QQuickItem *parent)
    {
        qInfo() << "mirror" << item <<"exported" << exportedItem << "source" << source << "parent" << parent;

        // Set parent first, otherwise export item suddenly appears at first.
        item->setParentItem(parent);

        qreal wRatio = getParentRatio(item, exportedItem);
        qreal hRatio = getParentRatio(item, exportedItem, false);

        item->setClip(exportedItem->clip());
        item->setX(exportedItem->x() * wRatio);
        item->setY(exportedItem->y() * hRatio);
        item->setZ(source->z());
        item->setWidth(source->width() * wRatio);
        item->setHeight(source->height() * hRatio);
    }

    // The first parameter is to avoid multiple inheritance of QQuickItem
    void setHandler(QQuickItem *item, QQuickItem *exportedItem, QQuickItem *source)
    {
        m_xConn = QObject::connect(exportedItem, &QQuickItem::xChanged, [item, exportedItem]() { item->setX(exportedItem->x() * getParentRatio(item, exportedItem)); });
        m_yConn = QObject::connect(exportedItem, &QQuickItem::yChanged, [item, exportedItem]() { item->setY(exportedItem->y() * getParentRatio(item, exportedItem, false)); });
        m_zConn = QObject::connect(source, &QQuickItem::zChanged, [item, source]() { item->setZ(source->z()); });
        m_widthConn = QObject::connect(source, &QQuickItem::widthChanged, [item, exportedItem, source]() { item->setWidth(source->width() * getParentRatio(item, exportedItem)); });
        m_heightConn = QObject::connect(source, &QQuickItem::heightChanged, [item, exportedItem, source]() { item->setHeight(source->height() * getParentRatio(item, exportedItem, false)); });
        // Triggered when the parent is deleted.
        m_parentConn = QObject::connect(item, &QQuickItem::parentChanged, [item]() { delete item; });
        // Triggered when source imported item is deleted.
        m_sourceConn = QObject::connect(source, &QObject::destroyed, [item]() { delete item; });
        m_parentWidthConn = QObject::connect(item->parentItem(), &QQuickItem::widthChanged, [item, exportedItem, source]() {
            qreal wRatio = getParentRatio(item, exportedItem);
            qreal hRatio = getParentRatio(item, exportedItem, false);

            item->setX(exportedItem->x() * wRatio);
            item->setY(exportedItem->y() * hRatio);
            item->setWidth(source->width() * wRatio);
            item->setHeight(source->height() * hRatio);
        });
        QWaylandQuickItem *wlItem = qobject_cast<QWaylandQuickItem *>(item);
        if (wlItem) {
            m_surfaceConn = QObject::connect(wlItem, &QWaylandQuickItem::surfaceChanged, [wlItem]() {
                qInfo() << wlItem << "is destroyed by changing surface";
                if (wlItem && !wlItem->surface())
                    delete wlItem;
            });
        }
    }

    void clearHandler()
    {
        QObject::disconnect(m_xConn);
        QObject::disconnect(m_yConn);
        QObject::disconnect(m_zConn);
        QObject::disconnect(m_widthConn);
        QObject::disconnect(m_heightConn);
        QObject::disconnect(m_parentConn);
        QObject::disconnect(m_sourceConn);
        QObject::disconnect(m_parentWidthConn);
        QObject::disconnect(m_surfaceConn);
    }

private:
    QMetaObject::Connection m_xConn;
    QMetaObject::Connection m_yConn;
    QMetaObject::Connection m_zConn;
    QMetaObject::Connection m_widthConn;
    QMetaObject::Connection m_heightConn;
    QMetaObject::Connection m_parentConn;
    QMetaObject::Connection m_sourceConn;
    QMetaObject::Connection m_parentWidthConn;
    QMetaObject::Connection m_surfaceConn;
};


class ImportedMirrorItem : public WebOSSurfaceItem, public MirrorItemHandler {
public:
    ImportedMirrorItem(WebOSCoreCompositor *compositor, QWaylandQuickSurface *surface)
        : WebOSSurfaceItem(compositor, surface)
    {
    }

    void initialize(QQuickItem *item, QQuickItem *exportedItem, QQuickItem *source, QQuickItem *parent) override
    {
        MirrorItemHandler::initialize(item, exportedItem, source, parent);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        // Keep mirrored item's size from being effected by internal reason of Qtwayland.
        // This can be removed in Qt6.
        static_cast<WebOSSurfaceItem *>(item)->setSizeFollowsSurface(false);
#endif
    }
};

class PunchThroughMirrorItem : public PunchThroughItem, public MirrorItemHandler {
};



WebOSForeign::WebOSForeign(WebOSCoreCompositor* compositor)
    : QtWaylandServer::wl_webos_foreign(compositor->display(), WEBOSFOREIGN_VERSION)
    , m_compositor(compositor)
{
}


void WebOSForeign::registeredWindow()
{
    if (m_compositor) {
        VideoOutputdCommunicator *pVideoOutputdCommunicator = VideoOutputdCommunicator::instance();
        if (pVideoOutputdCommunicator) {
            WebOSCompositorWindow *pWebOSCompositorWindow = qobject_cast<WebOSCompositorWindow *>(m_compositor->window());
            if (pWebOSCompositorWindow)
                pWebOSCompositorWindow->rootContext()->setContextProperty(QLatin1String("videooutputdCommunicator"), pVideoOutputdCommunicator);
            else
                qFatal("Failed to get the window instance");
        } else {
            qFatal("Failed to get VideoOutputdCommunicator instance");
        }

        VideoWindowInformer *pVideoWindowInformer = VideoWindowInformer::instance();
        if (pVideoWindowInformer) {
            WebOSCompositorWindow *pWebOSCompositorWindow = qobject_cast<WebOSCompositorWindow *>(m_compositor->window());
            if (pWebOSCompositorWindow)
                pWebOSCompositorWindow->rootContext()->setContextProperty(QLatin1String("videowindowInformer"), pVideoWindowInformer);
            else
                qFatal("Failed to get the window instance");
        } else {
            qFatal("Failed to get VideoOutputdCommunicator instance");
        }

    } else {
        qFatal("Cannot constructed without a compositor instance");
    }
}

WebOSExported* WebOSForeign::createExported(struct wl_client* client,
                                            uint32_t id, WebOSSurfaceItem* surfaceItem,
                                            WebOSExportedType exportedType)
{
    return new WebOSExported(this, client, id, surfaceItem, exportedType);
}

WebOSImported* WebOSForeign::createImported(WebOSExported *exported, struct wl_client *client,
                                            uint32_t id, WebOSExportedType exportedType)
{
    return new WebOSImported(exported, client, id, exportedType);
}

void WebOSForeign::webos_foreign_export_element(Resource *resource,
                                                uint32_t id,
                                                struct ::wl_resource *surface,
                                                uint32_t exported_type)
{
    QWaylandSurface* qwls = QWaylandSurface::fromResource(surface);
    QWaylandQuickSurface* quickSurface =
        static_cast<QWaylandQuickSurface*>(qwls);
    WebOSSurfaceItem *surfaceItem = WebOSSurfaceItem::getSurfaceItemFromSurface(quickSurface);
    WebOSExported *pWebOSExported =
        createExported(resource->client(), id, surfaceItem,
                       static_cast<WebOSForeign::WebOSExportedType>(exported_type));

    pWebOSExported->assignWindowId(generateWindowId());
    m_exportedList.append(pWebOSExported);
}

void WebOSForeign::webos_foreign_import_element(Resource *resource,
                                                uint32_t id,
                                                const QString &window_id,
                                                uint32_t exported_type)
{
    qInfo() << "webos_foreign_import_element with " << window_id;
    foreach(WebOSExported* exported, m_exportedList) {
        if (exported->m_windowId == window_id) {
            WebOSImported *imported =
                createImported(exported, resource->client(), id,
                               static_cast<WebOSForeign::WebOSExportedType>(exported_type));
            exported->m_importList.append(imported);
            return;
        }
    }
    wl_resource_post_error(resource->handle,
                           WL_DISPLAY_ERROR_INVALID_OBJECT,
                           "No matching WebOSExported.");
    wl_resource_destroy(resource->handle);
    qWarning() << "No matching WebOSExported with "
               << window_id;
}

WebOSExported::WebOSExported(
        WebOSForeign* foreign,
        struct wl_client* client,
        uint32_t id,
        WebOSSurfaceItem* surfaceItem,
        WebOSForeign::WebOSExportedType exportedType)
    : QtWaylandServer::wl_webos_exported(client, id, WEBOSEXPORTED_VERSION)
    , m_foreign(foreign)
    , m_surfaceItem(static_cast<WebOSSurfaceItem *>(surfaceItem))
    , m_exportedItem(new QQuickItem(surfaceItem))
    , m_exportedType(exportedType)
    , m_coverVideo(false)
    , m_activeRegion(QRect(0,0,0,0))
    , m_originalRequestedRegion(QRect(0,0,0,0))
{
    qInfo() << this << "is created";
    m_exportedItem->setClip(true);
    m_exportedItem->setZ(-1);
    m_exportedItem->setEnabled(false);
    m_surfaceItem->appendExported(this);
    updateCompositorWindow(m_surfaceItem->window());

    qInfo() <<"Window status of surface item (" << m_surfaceItem << ") for exporter : " << m_surfaceItem->state();

    m_isSurfaceItemFullscreen = (m_surfaceItem->state() == Qt::WindowFullScreen) ? true : false;

    connect(m_exportedItem, &QQuickItem::visibleChanged, this, &WebOSExported::updateVisible);
    connect(m_surfaceItem, &WebOSSurfaceItem::stateChanged, this, &WebOSExported::updateWindowState);
    connect(m_surfaceItem, &QWaylandQuickItem::xChanged, this, &WebOSExported::calculateAll);
    connect(m_surfaceItem, &QWaylandQuickItem::yChanged, this, &WebOSExported::calculateAll);
    connect(m_surfaceItem, &QWaylandQuickItem::widthChanged, this, &WebOSExported::calculateAll);
    connect(m_surfaceItem, &QWaylandQuickItem::heightChanged, this, &WebOSExported::calculateAll);
    connect(m_surfaceItem, &QWaylandQuickItem::scaleChanged, this, &WebOSExported::calculateAll);
    connect(m_surfaceItem, &QWaylandQuickItem::surfaceDestroyed, this, &WebOSExported::onSurfaceDestroyed);
    connect(m_surfaceItem, &WebOSSurfaceItem::itemAboutToBeDestroyed, this, &WebOSExported::onSurfaceDestroyed);
    connect(m_surfaceItem, &WebOSSurfaceItem::windowChanged, this, &WebOSExported::updateCompositorWindow);
    connect(m_surfaceItem, &WebOSSurfaceItem::coverStateChanged, this, &WebOSExported::updateCoverState);
    connect(m_surfaceItem, &WebOSSurfaceItem::activeRegionChanged, this, &WebOSExported::updateActiveRegion);
    connect(m_foreign->m_compositor, &WebOSCoreCompositor::surfaceMapped, this, &WebOSExported::onSurfaceItemMapped);

    updateCoverState();
    updateActiveRegion();
    calculateAll();

#if 0 // DEBUG
    QUrl debugUIQml = QUrl(QString("file://") +
                           QString(WEBOS_INSTALL_QML) +
                           QString("/WebOSCompositorBase/views/debug/exportedItemDebug.qml"));
    QQmlEngine* engine =
        (static_cast<WebOSCompositorWindow *>(compositor->window()))->engine();
    QQmlComponent component(engine, debugUIQml);
    QQuickItem *object = qobject_cast<QQuickItem*>(component.create());
    object->setParentItem(m_exportedItem);
    engine->setObjectOwnership(object, QQmlEngine::CppOwnership);
#endif
}

WebOSExported::~WebOSExported()
{
    qInfo() << "WebOSExported destructor is called on " << this;

    if (m_surfaceItem)
        m_surfaceItem->removeExported(this);

    foreach(WebOSImported* imported, m_importList) {
        imported->updateExported(nullptr);
    }

    while (!m_importList.isEmpty())
        m_importList.takeFirst()->destroyResource();

    // delete m_punchThroughItem in detach
    setPunchThrough(false);

    if (!m_muteRegisteredContextId.isNull()) {
        VideoOutputdCommunicator::instance()->setProperty("mute", "off", m_muteRegisteredContextId);
        VideoOutputdCommunicator::instance()->setProperty("registerMute", "off", m_muteRegisteredContextId);
        m_muteRegisteredContextId = QString();
    }

    m_foreign->m_exportedList.removeAll(this);

    // Usually it is deleted by QObjectPrivate::deleteChildren with its parent surfaceItem.
    if (m_exportedItem)
        delete m_exportedItem;
}

void WebOSExported::onSurfaceItemMapped(WebOSSurfaceItem *mappedItem)
{
    if (!mappedItem) {
        qWarning() << "mappedItem is null";
        return;
    }

    qDebug() << "mappedItem : " << mappedItem << " on " << m_windowId;
    if (m_surfaceItem == mappedItem) {
        qInfo() << "Surface item of exported is mapped on " << m_windowId;
        updateDisplayPosition(true);
    }
}

void WebOSExported::updateDisplayPosition(bool forceUpdate)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (!m_surfaceItem || m_surfaceItem->surface() == nullptr || m_surfaceItem->surface()->bufferSize().isValid() == false) {
        qDebug() << "SurfaceItem  for " << m_windowId << " is already destroyed";
        return;
    }

    QRectF globalPosition = m_surfaceItem->mapRectToScene(QRect(0,0,m_surfaceItem->surface()->bufferSize().width(), m_surfaceItem->surface()->bufferSize().height()));
#else
    if (!m_surfaceItem || m_surfaceItem->surface() == nullptr || m_surfaceItem->surface()->size().isValid() == false) {
        qDebug() << "SurfaceItem  for " << m_windowId << " is already destroyed";
        return;
    }

    QRectF globalPosition = m_surfaceItem->mapRectToScene(QRect(0,0,m_surfaceItem->surface()->size().width(), m_surfaceItem->surface()->size().height()));
#endif
    qDebug() << "globalPosition : " << globalPosition  << ", previous position : " << m_surfaceGlobalPosition << ", forceUpdate : " << forceUpdate << " on " << m_windowId;

    if (globalPosition != m_surfaceGlobalPosition || forceUpdate) {
        if (globalPosition.isValid()) {
            qInfo() << "globalPosition : " << globalPosition  << ", previous position : " << m_surfaceGlobalPosition << ", forceUpdate : " << forceUpdate << " on " << m_windowId;
            m_surfaceGlobalPosition = globalPosition;
            calculateVideoDispRatio();
        }
    }
}

void WebOSExported::calculateVideoDispRatio()
{
    qInfo() << "WebOSExported::calculateVideoDispRatio is called on " << m_windowId;
    if (!m_surfaceItem || !m_compositorWindow) {
        qWarning() << "WebOSSurfaceItem for" << m_windowId << "neither exists nor belongs to a window";
        return;
    }

    if (m_compositorWindow->outputGeometryPending()) {
        qWarning() << "OutputGeometry is being changed, should wait a bit more";
        return;
    }

    QRect outputGeometry = m_compositorWindow->outputGeometry();

    if (outputGeometry.isValid() && m_surfaceItem->surface()) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        if (!m_surfaceItem->surface()->bufferSize().isValid() || !m_surfaceGlobalPosition.isValid()) {
            m_videoDispRatio = 1.0;
            qInfo() << "video display ratio : " << m_videoDispRatio << ", surface size is not valid yet on " << m_windowId;
        } else {
            QRectF m_surfaceGlobalPosition = m_surfaceItem->mapRectToScene(QRect(0,0,m_surfaceItem->surface()->bufferSize().width(), m_surfaceItem->surface()->bufferSize().height()));

            if (!m_isSurfaceItemFullscreen) {
                m_videoDispRatio = qMin((double) m_surfaceGlobalPosition.width()/m_surfaceItem->surface()->bufferSize().width(), (double) m_surfaceGlobalPosition.height()/m_surfaceItem->surface()->bufferSize().height());
                qInfo() << "video display ratio : " << m_videoDispRatio << ", (not fullscreen case) surface global position : " << m_surfaceGlobalPosition << ", surface size : " << m_surfaceItem->surface()->bufferSize() << " on " << m_windowId;
            } else {
                QPointF offset = m_surfaceItem->mapToItem(m_compositorWindow->viewsRoot(), QPointF(0.0, 0.0));
                m_videoDispRatio = (double) outputGeometry.width() / m_surfaceItem->surface()->bufferSize().width() * m_surfaceItem->scale();
                qInfo() << "Output size:" << outputGeometry.size() << "surface size:" << m_surfaceItem->surface()->bufferSize() << "m_videoDispRatio:" << m_videoDispRatio;
#else
        if (!m_surfaceItem->surface()->size().isValid() || !m_surfaceGlobalPosition.isValid()) {
            m_videoDispRatio = 1.0;
            qInfo() << "video display ratio : " << m_videoDispRatio << ", surface size is not valid yet on " << m_windowId;
        } else {
            QRectF m_surfaceGlobalPosition = m_surfaceItem->mapRectToScene(QRect(0,0,m_surfaceItem->surface()->size().width(), m_surfaceItem->surface()->size().height()));

            if (!m_isSurfaceItemFullscreen) {
                m_videoDispRatio = qMin((double) m_surfaceGlobalPosition.width()/m_surfaceItem->surface()->size().width(), (double) m_surfaceGlobalPosition.height()/m_surfaceItem->surface()->size().height());
                qInfo() << "video display ratio : " << m_videoDispRatio << ", (not fullscreen case) surface global position : " << m_surfaceGlobalPosition << ", surface size : " << m_surfaceItem->surface()->size() << " on " << m_windowId;
            } else {
                QPointF offset = m_surfaceItem->mapToItem(m_compositorWindow->viewsRoot(), QPointF(0.0, 0.0));
                m_videoDispRatio = (double) outputGeometry.width() / m_surfaceItem->surface()->size().width() * m_surfaceItem->scale();
                qInfo() << "Output size:" << outputGeometry.size() << "surface size:" << m_surfaceItem->surface()->size() << "m_videoDispRatio:" << m_videoDispRatio;
#endif
            }
        }

        if (m_requestedRegion.isValid()) {
            setVideoDisplayRect();
            qInfo() << "Calculated video display output region:" << m_videoDisplayRect;
            setVideoDisplayWindow();
        } else {
           qWarning() << "Requested video region is not valid";
        }
    }
}

void WebOSExported::calculateExportedItemRatio()
{
    qInfo() << "WebOSExported::calculateExportedItemRatio is called on" << m_windowId;
    if (!m_surfaceItem || !m_compositorWindow) {
        qWarning() << "WebOSSurfaceItem for" << m_windowId << "neither exists nor belongs to a window";
        return;
    }

    if (m_compositorWindow->outputGeometryPending()) {
        qWarning() << "OutputGeometry is being changed, should wait a bit more";
        return;
    }

    QRect outputGeometry = m_compositorWindow->outputGeometry();

    if (m_isSurfaceItemFullscreen && outputGeometry.isValid()) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        m_exportedWindowRatio = (double) m_surfaceItem->width() / m_surfaceItem->surface()->bufferSize().width();
        qInfo() << "surface size: " << m_surfaceItem->surface()->bufferSize() << "item size:" << m_surfaceItem->size() << "m_exportedWindowRatio:" << m_exportedWindowRatio;
#else
        m_exportedWindowRatio = (double) m_surfaceItem->width() / m_surfaceItem->surface()->size().width();
        qInfo() << "surface size: " << m_surfaceItem->surface()->size() << "item size:" << m_surfaceItem->size() << "m_exportedWindowRatio:" << m_exportedWindowRatio;
#endif
        if (m_requestedRegion.isValid()) {
            m_destinationRect.setX((int)(m_requestedRegion.x()*m_exportedWindowRatio));
            m_destinationRect.setY((int)(m_requestedRegion.y()*m_exportedWindowRatio));
            m_destinationRect.setWidth((int)(m_requestedRegion.width()*m_exportedWindowRatio));
            m_destinationRect.setHeight((int)(m_requestedRegion.height()*m_exportedWindowRatio));
        }
        updateExportedItemSize();
    }
}

void WebOSExported::calculateAll()
{
    updateDisplayPosition(true);
    calculateExportedItemRatio();
}

void WebOSExported::updateWindowState()
{
    if (!m_surfaceItem) {
        qWarning() << "WebOSSurfaceItem for " << m_windowId << " is already destroyed";
        return;
    }
    WebOSSurfaceItem *item = qobject_cast<WebOSSurfaceItem*>(m_surfaceItem);
    qInfo() << "update window state : " << item->state() << "for WebOSExported ( " << m_windowId << " ) ";
    m_isSurfaceItemFullscreen = item->state() == Qt::WindowFullScreen;
    if (m_isSurfaceItemFullscreen) {
        calculateVideoDispRatio();
        calculateExportedItemRatio();
    }
}

void WebOSExported::updateCoverState()
{
    if (!m_surfaceItem) {
        qWarning() << "WebOSSurfaceItem for " << m_windowId << "is already destroyed";
        return;
    }

    bool coverVideo = false;
    if (m_surfaceItem->coverState() == WebOSSurfaceItem::CoverStateHidden) {
        coverVideo = true;
    }

    if (m_coverVideo != coverVideo) {
        qInfo() << "cover state is changed = " << coverVideo << " for WebOSExported (" << m_windowId << ")";
        m_coverVideo = coverVideo;
        calculateVideoDispRatio();
    }
}

void WebOSExported::updateActiveRegion()
{
    if (!m_surfaceItem) {
        qWarning() << "WebOSSurfaceItem for " << m_windowId << "is already destroyed";
        return;
    }

    QRect activeRegion = m_surfaceItem->activeRegion();

    if (m_activeRegion != activeRegion) {
        qInfo() << "active region is changed = " << activeRegion << " for WebOSExported (" << m_windowId << ")";
        m_activeRegion = activeRegion;
        updateDestinationRegionByActiveRegion();

        setDestinationRect();
        setVideoDisplayRect();

        qInfo() << "exported requested destination region : " << m_requestedRegion << " on " << m_windowId;
        qInfo() << "exported item region : " << m_destinationRect << ", video display region : " << m_videoDisplayRect;
        qInfo() << "exported item ratio : " << m_exportedWindowRatio << ", video display ratio :  " << m_videoDispRatio;

        setVideoDisplayWindow();
        updateExportedItemSize();
    }
}

void WebOSExported::updateVisible()
{
    if (!m_exportedItem) {
        qWarning() << "QWaylandQuickItem for " << m_windowId << " is  already destroyed ";
        return;
    }
    if (!m_contextId.isNull()) {
        if (m_exportedItem->isVisible()) {
            qInfo() << "exported item's visible is changed to true on " << m_windowId;
            updateVideoWindowList(m_contextId, m_videoDisplayRect, false);
        } else {
            qInfo() << "exported item's visible is changed to false on" << m_windowId;
            updateVideoWindowList(m_contextId, QRect(0, 0, 0, 0), true);
        }
    }
}

void WebOSExported::onSurfaceDestroyed()
{
    qInfo() << "Surface item for (" << m_windowId << ") is destroyed";

    m_surfaceItem = nullptr;
    updateCompositorWindow(nullptr);

    if (m_exportedItem) {
        foreach(WebOSImported* imported, m_importList) {
            if (imported)
                imported->childSurfaceDestroyed();
        }
        delete m_exportedItem;
        m_exportedItem = nullptr;
    }

    if (m_punchThroughItem) {
        delete m_punchThroughItem;
        m_punchThroughItem = nullptr;
    }
}

void WebOSExported::updateVideoWindowList(QString contextId, QRect videoDisplayRect, bool needRemove)
{
    if (needRemove || !m_exportedItem) {
        VideoWindowInformer::instance()->removeVideoWindowList(contextId);
    } else {
        if(!m_contextId.isNull() && m_exportedItem->isVisible() && m_surfaceItem) {
            QString appId = m_surfaceItem ? m_surfaceItem->appId() : "";
            qreal scaleFactor = m_surfaceItem->scale();
            QRect appWindow;
            if (m_activeRegion.isValid()) {
                appWindow = QRect(
                    m_surfaceGlobalPosition.x() + m_activeRegion.x()*scaleFactor,
                    m_surfaceGlobalPosition.y() + m_activeRegion.y()*scaleFactor,
                    m_activeRegion.width()*scaleFactor,
                    m_activeRegion.height()*scaleFactor);
            } else {
                appWindow = QRect(
                    m_surfaceGlobalPosition.x(),
                    m_surfaceGlobalPosition.y(),
                    m_surfaceItem->width()*scaleFactor,
                    m_surfaceItem->height()*scaleFactor);
            }
            VideoWindowInformer::instance()->insertVideoWindowList(contextId, videoDisplayRect, m_windowId, appId, appWindow);
        }
    }
}

void WebOSExported::updateExportedItemSize()
{
    if (!m_exportedItem)
        qWarning() << "WebOSSurfaceItem for " << m_windowId << " is  already destroyed ";

    qInfo() << m_exportedItem << "fits to" << m_destinationRect;

    m_exportedItem->setX(m_destinationRect.x());
    m_exportedItem->setY(m_destinationRect.y());
    m_exportedItem->setWidth(m_destinationRect.width());
    m_exportedItem->setHeight(m_destinationRect.height());

    if (m_punchThroughItem) {
        qInfo()<< "punch through item size changed:  " << m_exportedItem->width() << "on " << m_windowId;
        m_punchThroughItem->setWidth(m_exportedItem->width());
        m_punchThroughItem->setHeight(m_exportedItem->height());
    } else {
        if (m_exportedType == WebOSForeign::OpaqueObject) {
            setPunchThrough(true);
        }
    }
    emit geometryChanged();
}

void WebOSExported::setVideoDisplayWindow()
{
    QRect videoDisplayRect;
    if (m_foreign->m_compositor->window() && !m_contextId.isNull()) {
        if (m_coverVideo) {
            qInfo() << "cover video state. set video display rect = (0, 0, 0, 0)";
            videoDisplayRect = QRect(0, 0, 0, 0);
        } else {
            qDebug() << "Not cover state. Keep video display rect";
            videoDisplayRect = m_videoDisplayRect;
        }
        if (m_directVideoScalingMode) {
            qDebug() << "Direct video scaling mode is enabled. Do not call setDisplayWindow.";
        } else {
            if (m_originalInputRect.isValid())
               VideoOutputdCommunicator::instance()->setCropRegion(m_originalInputRect, m_sourceRect, m_videoDisplayRect, m_contextId);
            else
               VideoOutputdCommunicator::instance()->setDisplayWindow(m_sourceRect, m_videoDisplayRect, m_contextId);
        }
    } else {
        qInfo() << "Do not call setDisplayWindow. Punch through is not working";
    }

    if (!m_contextId.isNull())
        updateVideoWindowList(m_contextId, videoDisplayRect, false);
}

void WebOSExported::updateDestinationRegionByActiveRegion()
{
    bool isSetRegion = false;
    if (m_activeRegion.isValid()) {
        if (m_originalRequestedRegion.x() < m_activeRegion.x() || m_originalRequestedRegion.y() < m_activeRegion.y() ||
                m_originalRequestedRegion.x() + m_originalRequestedRegion.width() > m_activeRegion.x() + m_activeRegion.width() ||
	            m_originalRequestedRegion.y() + m_originalRequestedRegion.height() > m_activeRegion.y() + m_activeRegion.height()) {

                    if (m_originalRequestedRegion.isValid()) {
                        double scale = qMin((double)(m_activeRegion.width()) / (double)(m_originalRequestedRegion.width()), (double)(m_activeRegion.height()) / (double)(m_originalRequestedRegion.height()));
                        qInfo() << "Requested region is out of bounds of active region. scale = " << scale;

                        int x = (int)(m_activeRegion.x() + (m_activeRegion.width() - m_originalRequestedRegion.width()*scale)*0.5);
                        int y = (int)(m_activeRegion.y() + (m_activeRegion.height() - m_originalRequestedRegion.height()*scale)*0.5);
                        int width = (int)(m_originalRequestedRegion.width()*scale);
                        int height = (int)(m_originalRequestedRegion.height()*scale);
                        m_requestedRegion = QRect(x, y, width, height);
                        isSetRegion = true;
                    }
                }
	}

    if (isSetRegion == false)
        m_requestedRegion = m_originalRequestedRegion;
}

void WebOSExported::setDestinationRect() {
    m_destinationRect = QRect(
        (int) (m_requestedRegion.x()*m_exportedWindowRatio),
        (int) (m_requestedRegion.y()*m_exportedWindowRatio),
        (int) (m_requestedRegion.width()*m_exportedWindowRatio),
        (int) (m_requestedRegion.height()*m_exportedWindowRatio));
}

void WebOSExported::setVideoDisplayRect() {

    if (m_activeRegion.isValid()) {
        m_videoDisplayRect = QRect(
            round(m_surfaceGlobalPosition.x() + m_requestedRegion.x()*m_videoDispRatio),
            round(m_surfaceGlobalPosition.y() + m_requestedRegion.y()*m_videoDispRatio),
            round(m_requestedRegion.width()*m_videoDispRatio),
            round(m_requestedRegion.height()*m_videoDispRatio));
    } else {
        m_videoDisplayRect = QRect(
            (int) (m_surfaceGlobalPosition.x() + m_requestedRegion.x()*m_videoDispRatio),
            (int) (m_surfaceGlobalPosition.y() + m_requestedRegion.y()*m_videoDispRatio),
            (int) (m_requestedRegion.width()*m_videoDispRatio),
            (int) (m_requestedRegion.height()*m_videoDispRatio));
    }
}

void WebOSExported::setDestinationRegion(struct::wl_resource *destination_region)
{
    if (destination_region) {
        const QRegion qwlDestinationRegion =
            QtWayland::Region::fromResource(
                destination_region)->region();
        if (qwlDestinationRegion.boundingRect().isValid()) {
            m_originalRequestedRegion = QRect(
                qwlDestinationRegion.boundingRect().x(),
                qwlDestinationRegion.boundingRect().y(),
                qwlDestinationRegion.boundingRect().width(),
                qwlDestinationRegion.boundingRect().height());
        } else {
            m_originalRequestedRegion = QRect(0, 0, 0, 0);
        }

        updateDestinationRegionByActiveRegion();
        setDestinationRect();
        setVideoDisplayRect();

        qInfo() << "exported original requested destination region : " << m_originalRequestedRegion << " on " << m_windowId;
        qInfo() << "exported requested destination region : " << m_requestedRegion << " on " << m_windowId;
        qInfo() << "exported item region : " << m_destinationRect << ", video display region : " << m_videoDisplayRect;
        qInfo() << "exported item ratio : " << m_exportedWindowRatio << ", video display ratio :  " << m_videoDispRatio;

        setVideoDisplayWindow();
        updateExportedItemSize();
    }
}

void WebOSExported::updateCompositorWindow(QQuickWindow *window)
{
    if (window != m_compositorWindow) {
        if (m_compositorWindow)
            disconnect(m_compositorWindow, &WebOSCompositorWindow::outputGeometryChanged, this, &WebOSExported::calculateAll);
        m_compositorWindow = static_cast<WebOSCompositorWindow *>(window);
        connect(m_compositorWindow, &WebOSCompositorWindow::outputGeometryChanged, this, &WebOSExported::calculateAll);
    }
}

void WebOSExported::webos_exported_set_exported_window(
        Resource *resource,
        struct ::wl_resource *source_region,
        struct ::wl_resource *destination_region)
{
    Q_UNUSED(source_region);

    m_originalInputRect = QRect(0, 0, 0, 0);
    m_sourceRect = QRect(0, 0, 0, 0);

    setDestinationRegion(destination_region);
}

void WebOSExported::webos_exported_set_crop_region(
        Resource *resource,
        struct ::wl_resource *original_input,
        struct ::wl_resource *source_region,
        struct ::wl_resource *destination_region)
{
    if (original_input) {
        const QRegion qwlOriginalInput =
            QtWayland::Region::fromResource(original_input)->region();

        if (qwlOriginalInput.boundingRect().isValid()) {
            m_originalInputRect = QRect(
                qwlOriginalInput.boundingRect().x(),
                qwlOriginalInput.boundingRect().y(),
                qwlOriginalInput.boundingRect().width(),
                qwlOriginalInput.boundingRect().height());
        }
    }

    if (source_region) {
        const QRegion qwlSourceRegion =
            QtWayland::Region::fromResource(source_region)->region();

        if (qwlSourceRegion.boundingRect().isValid()) {
            m_sourceRect = QRect(
                qwlSourceRegion.boundingRect().x(),
                qwlSourceRegion.boundingRect().y(),
                qwlSourceRegion.boundingRect().width(),
                qwlSourceRegion.boundingRect().height());
        } else {
            m_sourceRect = QRect(0, 0, 0, 0);
        }
    }

    qInfo() << "crop_region original rect : " << m_originalInputRect << "on " << m_windowId;
    qInfo() << "crop_region source rect : " << m_sourceRect << "on " << m_windowId;

    setDestinationRegion(destination_region);
}

void WebOSExported::webos_exported_set_property(
        Resource *resource,
        const QString &name,
        const QString &value)
{
    qInfo() << "set_property name : " << name << " value : " << value << "on" << m_windowId;

    // Set exportedItems' z_index from z_index value
    if (name == "z_index") {
        bool result;
        int z_index = value.toInt(&result, 10);
        if (result) {
            qInfo() << "set exportedItem's z_index to " << z_index;
            m_exportedItem->setZ(z_index);
        } else {
            qInfo() << "Failed to convert value to integer";
        }
        return;
    }

    if (name == "directVideoScalingMode") {
        if (value == "on")
            m_directVideoScalingMode = true;
        else
            m_directVideoScalingMode = false;
        return;
    }

    if (name == "mute") {
        if (m_properties.find("mute") == m_properties.end()) {
            if (m_foreign->m_compositor->window() && !m_contextId.isNull()) {
                VideoOutputdCommunicator::instance()->setProperty("registerMute", "on", m_contextId);
                m_muteRegisteredContextId = m_contextId;
            } else {
                m_properties.insert("registerMute", "on");
            }
        }
    }

    if (m_foreign->m_compositor->window()) {
        if (name == "mute" && value == "windowBasedOn") {
            VideoOutputdCommunicator::instance()->setProperty(name, "on", m_contextId);
        } else {
            VideoOutputdCommunicator::instance()->setProperty(name, value, m_contextId);
        }
        if (value == "none") {
            m_properties.remove(name);
            return;
        }
    } else {
        qInfo() << "Do not call setProperty as there is no window for this WebOSExported" << this;
    }

    if (value.isNull())
        m_properties.remove(name);
    else
        m_properties.insert(name, value);
}

void WebOSExported::setPunchThrough(bool needPunch)
{
    if (needPunch && m_exportedItem) {
        if (!m_punchThroughItem && m_exportedType != WebOSForeign::TransparentObject) {
            PunchThroughItem* punchThroughNativeItem =
                new PunchThroughItem();
            punchThroughNativeItem->setParentItem(m_exportedItem);
            punchThroughNativeItem->setZ(m_exportedItem->z()-1);
            punchThroughNativeItem->setWidth(m_destinationRect.width());
            punchThroughNativeItem->setHeight(m_destinationRect.height());
            m_punchThroughItem = punchThroughNativeItem;
        }
    } else {
        if (m_punchThroughItem && m_exportedType != WebOSForeign::OpaqueObject) {
            m_punchThroughItem->setX(0);
            m_punchThroughItem->setY(0);
            m_punchThroughItem->setWidth(0);
            m_punchThroughItem->setHeight(0);
            m_punchThroughItem->setVisible(false);
            delete m_punchThroughItem;
            m_punchThroughItem = nullptr;
        }
    }
}

void WebOSExported::assignWindowId(QString windowId)
{
    m_windowId = windowId;
    qInfo() << m_windowId << "is assigned for " << this;
    send_window_id_assigned(m_windowId, m_exportedType);
}

WebOSSurfaceItem *WebOSExported::getImportedItem()
{
    QQuickItem *childDisplayItem = m_exportedItem && !m_exportedItem->childItems().isEmpty() ? m_exportedItem->childItems().first() : nullptr;

    if (!childDisplayItem || childDisplayItem->childItems().isEmpty())
        return nullptr;

    if (childDisplayItem->childItems().size() > 1)
        qWarning() << "more than one imported item for WebOSExported" << m_surfaceItem;

    // Imported surface item
    return static_cast<WebOSSurfaceItem *>(childDisplayItem->childItems().first());
}

bool WebOSExported::hasSecuredContent()
{
    if (!m_exportedItem || m_exportedItem->childItems().isEmpty())
        return false;

    foreach(QQuickItem *item, m_exportedItem->childItems()) {
        WebOSSurfaceItem *surfaceItem = qobject_cast<WebOSSurfaceItem *>(item);
        if (surfaceItem && surfaceItem->view()) {
            if (surfaceItem->view()->currentBuffer().hasProtectedContent())
                return true;
        }
    }

    return false;
}

void WebOSExported::setParentOf(QQuickItem *item, QQuickItem *childDisplayItem)
{
    if (!m_surfaceItem || !m_exportedItem) {
        qWarning() << "unexpected null reference" << m_surfaceItem << m_exportedItem;
        return;
    }

    item->setParentItem(childDisplayItem);

    // mirrored items
    foreach(WebOSSurfaceItem *parent, m_surfaceItem->mirrorItems())
        startImportedMirroring(parent);
}

void WebOSExported::registerMuteOwner(const QString& contextId)
{
    qDebug() << "WebOSExported::registerMuteOwner() m_muteRegisteredContextId : " << m_muteRegisteredContextId <<  "contextId: " << contextId;

    if (!m_muteRegisteredContextId.isNull()) {
        if (m_muteRegisteredContextId == contextId) {
            qDebug() << "ContextId is same. Do not unregister mute owner";
        } else {
            qDebug() << "ContextId is different from the previous one";
            VideoOutputdCommunicator::instance()->setProperty("mute", "off", m_muteRegisteredContextId);
            VideoOutputdCommunicator::instance()->setProperty("registerMute", "off", m_muteRegisteredContextId);
            VideoOutputdCommunicator::instance()->setProperty("registerMute", "on", contextId);
            m_muteRegisteredContextId = contextId;
        }
    } else {
        qDebug() << "m_muteRegisteredContextId is null";
        QMap<QString, QString>::const_iterator it = (m_properties).find("registerMute");
        if (it!= (m_properties).end()) {
            VideoOutputdCommunicator::instance()->setProperty(it.key(), it.value(), contextId);
            m_properties.remove(it.key());
            m_muteRegisteredContextId = contextId;
        }
    }
}

void WebOSExported::unregisterMuteOwner()
{
    if (!m_contextId.isNull()) {
        QMap<QString, QString>::const_iterator it = m_properties.find("mute");
        if (it!= m_properties.end()) {
            if (it.value() == "on")
                VideoOutputdCommunicator::instance()->setProperty(it.key(), "off", m_contextId);
            if (it.value() != "windowBasedOn") {
                m_properties.remove(it.key());
                VideoOutputdCommunicator::instance()->setProperty("registerMute", "off", m_contextId);
                m_muteRegisteredContextId = QString();
            }
        }
    }
}

void WebOSExported::webos_exported_destroy(Resource *r)
{
    qInfo() << "webos_exported_destroy is called on " << this;
    if (r)
        wl_resource_destroy(r->handle);
}

void WebOSExported::webos_exported_destroy_resource(Resource *r)
{
    Q_UNUSED(r);
    delete this;
}

void WebOSExported::startImportedMirroring(WebOSSurfaceItem *parent)
{
    if (!parent)
        return;

    QQuickItem *source = getImportedItem();

    qInfo() << "startImportedMirroring for source" << source;

    // Nothing imported to the exported item
    if (!source)
        return;

    if (m_punchThroughItem) {
        PunchThroughMirrorItem *mirror = new PunchThroughMirrorItem();
        mirror->initialize(mirror, m_exportedItem, source, parent);
        mirror->setHandler(mirror, m_exportedItem, source);
    } else {
        WebOSSurfaceItem *si = static_cast<WebOSSurfaceItem *>(source);
        ImportedMirrorItem *mirror = new ImportedMirrorItem(m_foreign->m_compositor, static_cast<QWaylandQuickSurface *>(si->surface()));
        mirror->initialize(mirror, m_exportedItem, source, parent);
        mirror->setHandler(mirror, m_exportedItem, source);
        mirror->setImported(si->imported());
        mirror->setDirectUpdateOnPlane(parent->directUpdateOnPlane());
        connect(parent, &WebOSSurfaceItem::directUpdateOnPlaneChanged, mirror, &WebOSSurfaceItem::updateDirectUpdateOnPlane);

        qInfo() << "source" << si << "mirror" << mirror;
    }
}

WebOSImported::WebOSImported(WebOSExported* exported, struct wl_client* client,
                        uint32_t id, WebOSForeign::WebOSExportedType exportedType)
    : QtWaylandServer::wl_webos_imported(client, id,
                                         WEBOSIMPORTED_VERSION)
    , m_exported(exported)
    , m_importedType(exportedType)
{
    if (exported) {
        qInfo() << this << "is created. Video window id is : " << m_exported->m_windowId;
        connect(exported, &WebOSExported::geometryChanged,
                this, &WebOSImported::updateGeometry);
        send_destination_region_changed(exported->m_destinationRect.width(),
                                        exported->m_destinationRect.height());
    } else {
        qWarning() << this << "is created with a null video window";
    }
}

WebOSImported::~WebOSImported()
{
    qInfo() << "WebOSImported destructor is called on " << this;
    if (!m_exported)
        return;
    else
        m_exported->m_importList.removeAll(this);

    detach();
}

void WebOSImported::destroyResource()
{
    // When wl_exported deleted...
    foreach(Resource* resource, resourceMap().values()) {
        wl_resource_destroy(resource->handle);
    }
}

void WebOSImported::setSurfaceItemSize()
{
    if (m_childSurfaceItem && m_childDisplayItem && m_exported && m_exported->m_exportedItem) {
        if (m_textureAlign != WebOSImported::surface_alignment::surface_alignment_crop ||
                !m_exported->m_originalInputRect.isValid()) {
            qInfo() << "set surface item's width : " << m_exported->m_exportedItem->width() << this;
            qInfo() << "set surface item's height : " << m_exported->m_exportedItem->height() << this;

            m_childDisplayItem->setWidth(m_exported->m_exportedItem->width());
            m_childDisplayItem->setHeight(m_exported->m_exportedItem->height());

            m_childSurfaceItem->setWidth(m_childDisplayItem->width());
            m_childSurfaceItem->setHeight(m_childDisplayItem->height());
            m_childSurfaceItem->setX(0);
            m_childSurfaceItem->setY(0);
        } else {
            double widthRatio = double(m_exported->m_destinationRect.width()) / double(m_exported->m_sourceRect.width());
            double heightRatio = double(m_exported->m_destinationRect.height()) / double(m_exported->m_sourceRect.height());

            qInfo() << "Crop surface item's destination : "
                            << m_exported->m_destinationRect.width() << "x"
                            << m_exported->m_destinationRect.height() << this;
            qInfo() << "Crop surface item's coord : "
                            << (int)((m_exported->m_originalInputRect.x() - m_exported->m_sourceRect.x()) * widthRatio) << ","
                            << (int)((m_exported->m_originalInputRect.y() - m_exported->m_sourceRect.y()) * heightRatio) << ","
                            << (int)(m_exported->m_originalInputRect.width() * widthRatio) << "x"
                            << (int)(m_exported->m_originalInputRect.height() * heightRatio) << widthRatio << heightRatio << this;

            m_childDisplayItem->setWidth(m_exported->m_destinationRect.width());
            m_childDisplayItem->setHeight(m_exported->m_destinationRect.height());

            m_childSurfaceItem->setWidth((int)(m_exported->m_originalInputRect.width() * widthRatio));
            m_childSurfaceItem->setHeight((int)(m_exported->m_originalInputRect.height() * heightRatio));
            m_childSurfaceItem->setX((int)((m_exported->m_originalInputRect.x() - m_exported->m_sourceRect.x()) * widthRatio));
            m_childSurfaceItem->setY((int)((m_exported->m_originalInputRect.y() - m_exported->m_sourceRect.y()) * heightRatio));
        }
        //TODO: handle other cases.
    }
}

void WebOSImported::updateGeometry()
{
    if (m_childSurfaceItem) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
            connect(m_childSurfaceItem->surface(), &QWaylandSurface::bufferSizeChanged, this, &WebOSImported::setSurfaceItemSize);
#else
            connect(m_childSurfaceItem->surface(), &QWaylandSurface::sizeChanged, this, &WebOSImported::setSurfaceItemSize, Qt::UniqueConnection);
#endif
    }
    setSurfaceItemSize();

    if (m_exported && m_exported->m_exportedItem) {
        send_destination_region_changed(m_exported->m_exportedItem->width(),
                                    m_exported->m_exportedItem->height());
    }
}

void WebOSImported::detach()
{
    qWarning() << "detach is called on  : " << this;
    if (m_exported) {
        // exported for punch through is destroyed
        if (m_punchThroughAttached) {
            qInfo() << "request to detach punchthrough";
            webos_imported_detach_punchthrough(nullptr);
        }

       // exported for texture surface is destroyed
        if (m_childSurfaceItem) {
            qInfo() << "request to detach surface";
            webos_imported_detach_surface(nullptr, m_childSurfaceItem->surface()->resource());
        }
    }
}

void WebOSImported::updateExported(WebOSExported * exported)
{
    qInfo() << "WebOSImported::updateExported is called " << this;

    if (m_exported == exported)
        return;

    if (exported == nullptr)
        detach();

    m_exported = exported;
}

void WebOSImported::webos_imported_attach_punchthrough(Resource* r)
{
    qInfo() << "attach_punchthrough is called, invoke attach_punchthrough_with_context with MAIN context for backward compatibility";
    webos_imported_attach_punchthrough_with_context(r, QString("MAIN"));
}

void WebOSImported::webos_imported_attach_punchthrough_with_context(Resource* r, const QString& contextId)
{
    Q_UNUSED(r);

    qInfo() << "attach_punchthrough_with_context is called with contextId " << contextId << " on " << this;

    if (!m_exported || !(m_exported->m_exportedItem) || contextId.isNull()) {
        qWarning() << " Fail to attach punch through (m_exported : " << m_exported << " contextId : " << contextId << ")";
        return;
    }
    if (!(m_exported->m_contextId.isNull()))
        qWarning() << "m_exported has m_contextId " << m_exported->m_contextId;

    m_exported->registerMuteOwner(contextId);

    // set_property
    if (!(m_exported->m_properties).isEmpty()) {
        QMap<QString, QString>::const_iterator i = m_exported->m_properties.constBegin();
        while (i != m_exported->m_properties.constEnd()) {
            qInfo() << "m_properties key : " << i.key() << " value :  " << i.value();
            if (i.key() == "mute" && i.value() == "windowBasedOn")
                VideoOutputdCommunicator::instance()->setProperty(i.key(), "on", contextId);
            else
                VideoOutputdCommunicator::instance()->setProperty(i.key(), i.value(), contextId);
            ++i;
        }
    }

    m_contextId = contextId;
    m_exported->m_contextId = contextId;
    m_punchThroughAttached = true;

    if (m_exported->m_requestedRegion.isValid())
        m_exported->setVideoDisplayWindow();

    m_exported->updateDisplayPosition(true);
    m_exported->setPunchThrough(true);
    send_punchthrough_attached(contextId);
}

void WebOSImported::webos_imported_detach_punchthrough(Resource* r)
{
    Q_UNUSED(r);

    qInfo() << "detach_punchthrough is called on " << this;

    if (!m_exported || m_exported->m_contextId.isNull()) {
        qWarning() << "m_exported or m_contextId is null";
        return;
    }

    if (m_contextId == m_exported->m_contextId) {
        qInfo() << "detach_punchthrough is called for m_exported->contextId " << m_exported->m_contextId;
        m_exported->unregisterMuteOwner();
        m_exported->setPunchThrough(false);
        send_punchthrough_detached(m_exported->m_contextId);
        m_exported->updateVideoWindowList(m_exported->m_contextId, QRect(0, 0, 0, 0), true);
        m_exported->m_contextId = QString();
        m_punchThroughAttached = false;
    } else {
        qInfo() << "detach_punchthrough is called for contextId " << m_contextId;
        send_punchthrough_detached(m_contextId);
        m_exported->updateVideoWindowList(m_contextId, QRect(0, 0, 0, 0), true);
        m_punchThroughAttached = false;
    }
}

void WebOSImported::webos_imported_destroy(Resource* r)
{
    qInfo() << "webos_imported_destroy is called on " << this;
    if (r)
        wl_resource_destroy(r->handle);
}

void WebOSImported::webos_imported_destroy_resource(Resource* r)
{
    Q_UNUSED(r);
    delete this;
}

void WebOSImported::destroyChildDisplay()
{
    qInfo() << "destroyChildDisplay is called on " << this;
    if (m_childDisplayItem) {
        delete m_childDisplayItem;
        m_childDisplayItem = nullptr;
    }
}

void WebOSImported::childSurfaceDestroyed()
{
    qInfo() << "childSurfaceDestroyed is called on " << this;
    if (m_childSurfaceItem)
        m_childSurfaceItem = nullptr;
    destroyChildDisplay();
}

void WebOSImported::webos_imported_attach_surface(
        Resource* resource,
        struct ::wl_resource* surface)
{

    if (!m_exported || !(m_exported->m_exportedItem) || !surface) {
        qWarning() << "Exported (" << m_exported << " ) is already destroyed or surface (" << surface << ") is null";
        return;
    }
    auto qwlSurface = QWaylandSurface::fromResource(surface);

    qInfo() << qwlSurface << "from" << surface;

    m_childSurfaceItem = WebOSSurfaceItem::getSurfaceItemFromSurface(qwlSurface);
    if (!m_childDisplayItem) {
        m_childDisplayItem = new QQuickItem(m_exported->m_exportedItem);
    }
    m_childDisplayItem->setClip(true);
    connect(m_childSurfaceItem->surface(), &QWaylandSurface::surfaceDestroyed, this, &WebOSImported::childSurfaceDestroyed);
    m_childSurfaceItem->setImported(true);
    // Applying direct update after the child surface item belongs to a window
    m_childSurfaceItem->setDirectUpdateOnPlane(m_exported->surfaceItem()->directUpdateOnPlane());
    connect(m_exported->surfaceItem(), &WebOSSurfaceItem::directUpdateOnPlaneChanged, m_childSurfaceItem, &WebOSSurfaceItem::updateDirectUpdateOnPlane);
    m_exported->setParentOf(m_childSurfaceItem, m_childDisplayItem);
    m_childDisplayItem->setZ(m_exported->m_exportedItem->z()+m_z_index);
    updateGeometry();  //Resize texture if needed.
    if (m_importedType == WebOSForeign::WebOSExportedType::VideoObject)
        VideoOutputdCommunicator::instance()->setProperty("videoTexture", "on", NULL);
    send_surface_attached(surface);
}

void WebOSImported::webos_imported_detach_surface(
        Resource * resource,
        struct ::  wl_resource *surface)
{
    qInfo() <<"detach_surface is called : " << surface << " on " << this;

    if (!m_childSurfaceItem || m_childSurfaceItem->surface()->resource() != surface) {
        qWarning() << "surface is not the attached surface";
        return;
    }

    if (m_importedType == WebOSForeign::WebOSExportedType::VideoObject)
        VideoOutputdCommunicator::instance()->setProperty("videoTexture", "off", NULL);
    disconnect(m_childSurfaceItem->surface(), &QWaylandSurface::surfaceDestroyed, this, &WebOSImported::childSurfaceDestroyed);
    m_childSurfaceItem->setParentItem(nullptr);
    if (m_childDisplayItem)
        m_childDisplayItem->setParentItem(nullptr);
    send_surface_detached(m_childSurfaceItem->surface()->resource());
    childSurfaceDestroyed();
}

void WebOSImported::webos_imported_set_z_index(
        Resource * resource,
        int32_t z_index)
{
    if (m_z_index != z_index) {
        m_z_index = z_index;
        qInfo() << "z_index of WebOSImported (" << this << " ) is changed to " << z_index;
    }
}

void WebOSImported::webos_imported_set_surface_alignment(
        Resource * resource,
        uint32_t surface_alignment)
{
    if (m_textureAlign != (WebOSImported::surface_alignment)surface_alignment) {
        m_textureAlign = (WebOSImported::surface_alignment)surface_alignment;
        qInfo() << "m_textureAlign of WebOSImported (" << this << " ) is changed to " << surface_alignment;
        setSurfaceItemSize();
    }
}
