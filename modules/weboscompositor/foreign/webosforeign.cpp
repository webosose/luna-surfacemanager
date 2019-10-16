// Copyright (c) 2018-2019 LG Electronics, Inc.
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
#include "webossurfaceitem.h"
#include "videowindow_informer.h"

#include <QDebug>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWaylandCompositor>
#include <QWaylandSurfaceItem>

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

    void initialize(QQuickItem *item, QQuickItem *exportedItem, QQuickItem *parent)
    {
        item->setClip(exportedItem->clip());
        item->setZ(exportedItem->z());
        item->setX(exportedItem->x());
        item->setY(exportedItem->y());
        item->setWidth(exportedItem->width());
        item->setHeight(exportedItem->height());
        item->setParentItem(parent);
    }

    // The first parameter is to avoid multiple inheritance of QQuickItem
    void setHandler(QQuickItem *item, QQuickItem *exportedItem, QQuickItem *source)
    {
        m_xConn = QObject::connect(exportedItem, &QQuickItem::xChanged, [item, exportedItem]() { item->setX(exportedItem->x()); });
        m_yConn = QObject::connect(exportedItem, &QQuickItem::yChanged, [item, exportedItem]() { item->setY(exportedItem->y()); });
        m_widthConn = QObject::connect(exportedItem, &QQuickItem::widthChanged, [item, exportedItem]() { item->setWidth(exportedItem->width()); });
        m_heightConn = QObject::connect(exportedItem, &QQuickItem::heightChanged, [item, exportedItem]() { item->setHeight(exportedItem->height()); });
        // Triggered when the parent is deleted.
        m_parentConn = QObject::connect(item, &QQuickItem::parentChanged, [item]() { delete item; });
        // Triggered when source imported item is deleted.
        m_sourceConn = QObject::connect(source, &QObject::destroyed, [item]() { delete item; });
    }

    void clearHandler()
    {
        QObject::disconnect(m_xConn);
        QObject::disconnect(m_yConn);
        QObject::disconnect(m_widthConn);
        QObject::disconnect(m_heightConn);
        QObject::disconnect(m_parentConn);
        QObject::disconnect(m_sourceConn);
    }

private:
    QMetaObject::Connection m_xConn;
    QMetaObject::Connection m_yConn;
    QMetaObject::Connection m_widthConn;
    QMetaObject::Connection m_heightConn;
    QMetaObject::Connection m_parentConn;
    QMetaObject::Connection m_sourceConn;
};


class ImportedMirrorItem : public QWaylandSurfaceItem, public MirrorItemHandler {
public:
    ImportedMirrorItem(QWaylandQuickSurface *surface) : QWaylandSurfaceItem(surface) {}
};

class PunchThroughMirrorItem : public PunchThroughItem, public MirrorItemHandler {
};



WebOSForeign::WebOSForeign(WebOSCoreCompositor* compositor)
    : QtWaylandServer::wl_webos_foreign(compositor->waylandDisplay(), WEBOSFOREIGN_VERSION)
    , m_compositor(compositor)
{
}


void WebOSForeign::registeredWindow()
{
    if (m_compositor) {
        VideoOutputdCommunicator *pVideoOutputdCommunicator = VideoOutputdCommunicator::instance();
        if (pVideoOutputdCommunicator) {
            WebOSCompositorWindow *pWebOSCompositorWindow = qobject_cast<WebOSCompositorWindow *>(m_compositor->window());
            if (pWebOSCompositorWindow) {
                pWebOSCompositorWindow->rootContext()->setContextProperty(QLatin1String("videooutputdCommunicator"), pVideoOutputdCommunicator);
                m_outputGeometry = pWebOSCompositorWindow->outputGeometry();
            } else {
                qFatal("Failed to get the window instance");
            }
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

void WebOSForeign::webos_foreign_export_element(Resource *resource,
                                                uint32_t id,
                                                struct ::wl_resource *surface,
                                                uint32_t exported_type)
{
    QtWayland::Surface* qwls = QtWayland::Surface::fromResource(surface);
    QWaylandQuickSurface* quickSurface =
        static_cast<QWaylandQuickSurface*>(qwls->waylandSurface());
    WebOSExported *pWebOSExported =
        new WebOSExported(this, resource->client(), id,
                          quickSurface->surfaceItem(),
                          (WebOSForeign::WebOSExportedType)exported_type);

    pWebOSExported->assigneWindowId(generateWindowId());
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
            exported->m_importList.append(
                new WebOSImported(exported, resource->client(), id));
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
        QWaylandSurfaceItem* surfaceItem,
        WebOSForeign::WebOSExportedType exportedType)
    : QtWaylandServer::wl_webos_exported(client, id, WEBOSEXPORTED_VERSION)
    , m_foreign(foreign)
    , m_qwlsurfaceItem(static_cast<WebOSSurfaceItem *>(surfaceItem))
    , m_exportedItem(new QQuickItem(surfaceItem))
    , m_exportedType(exportedType)
{
    qInfo() << this << "is created";
    m_exportedItem->setClip(true);
    m_exportedItem->setZ(-1);
    m_qwlsurfaceItem->setExported(this);

    WebOSSurfaceItem *item = qobject_cast<WebOSSurfaceItem*>(surfaceItem);
    qInfo() <<"Window status of surface item (" << item << ") for exporter : " <<item->state();
    m_isSurfaceItemFullscreen = (item->state() == Qt::WindowFullScreen) ? true : false;

    connect(m_exportedItem, &QQuickItem::visibleChanged, this, &WebOSExported::updateVisible);
    connect(item, &WebOSSurfaceItem::stateChanged, this, &WebOSExported::updateWindowState);
    connect(m_qwlsurfaceItem, &QWaylandSurfaceItem::widthChanged, this, &WebOSExported::calculateExportedItemRatio);
    connect(m_qwlsurfaceItem, &QWaylandSurfaceItem::surfaceDestroyed, this, &WebOSExported::onSurfaceDestroyed);
    connect(item, &WebOSSurfaceItem::surfaceAboutToBeDestroyed, this, &WebOSExported::onSurfaceDestroyed);

    calculateVideoDispRatio();
    calculateExportedItemRatio();

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

    if (m_qwlsurfaceItem)
        m_qwlsurfaceItem->setExported(nullptr);

    foreach(WebOSImported* imported, m_importList) {
        imported->updateExported(nullptr);
    }

    while (!m_importList.isEmpty())
        m_importList.takeFirst()->destroyResource();

    // delete m_punchThroughItem in detach
    setPunchThrough(false);

    m_foreign->m_exportedList.removeAll(this);

    // Usually it is deleted by QObjectPrivate::deleteChildren with its parent surfaceItem.
    if (m_exportedItem)
        delete m_exportedItem;
}

void WebOSExported::calculateVideoDispRatio()
{
    qInfo() << "WebOSExported::calculateVideoDispRatio is called on " << m_windowId;
    if (!m_qwlsurfaceItem) {
        qWarning() << "QWaylandSurfaceItem  for " << m_windowId << " is already destroyed";
        return;
    }

    if (m_isSurfaceItemFullscreen && m_foreign->m_outputGeometry.isValid() && m_qwlsurfaceItem->surface()) {
        m_videoDispRatio = (double) m_foreign->m_outputGeometry.width() / m_qwlsurfaceItem->surface()->size().width();
        qInfo() << "surface geometry : " << m_qwlsurfaceItem->surface()->size().width() << "x" << m_qwlsurfaceItem->surface()->size().height();
        qInfo() <<"m_videoDispRatio : " <<m_videoDispRatio;
        if (m_requestedRegion.isValid() && m_videoDisplayRect.isValid()) {
            m_videoDisplayRect.setX((int) (m_requestedRegion.x()*m_videoDispRatio));
            m_videoDisplayRect.setY((int) (m_requestedRegion.y()*m_videoDispRatio));
            m_videoDisplayRect.setWidth((int) (m_requestedRegion.width()*m_videoDispRatio));
            m_videoDisplayRect.setHeight((int) (m_requestedRegion.height()*m_videoDispRatio));

            setVideoDisplayWindow();
        }
    }
}

void WebOSExported::calculateExportedItemRatio()
{
    qInfo() << "WebOSExported::calculateExportedItemRatio is called on " << m_windowId;
    if (!m_qwlsurfaceItem) {
        qWarning() << "QWaylandSurfaceItem  for " << m_windowId << " is  already destroyed";
        return;
    }

    if (m_isSurfaceItemFullscreen && m_foreign->m_outputGeometry.isValid()) {
        m_exportedWindowRatio = (double) m_qwlsurfaceItem->width() / m_qwlsurfaceItem->surface()->size().width();
        qInfo() << "surface geometry : " << m_qwlsurfaceItem->surface()->size().width() << "x" << m_qwlsurfaceItem->surface()->size().height();
        qInfo() << "surface item geometry : " << m_qwlsurfaceItem->width() << "x" << m_qwlsurfaceItem->height();
        qInfo() <<"m_exportedWindowRatio : " <<m_exportedWindowRatio;
        if (m_requestedRegion.isValid()) {
            m_destinationRect.setX((int)(m_requestedRegion.x()*m_exportedWindowRatio));
            m_destinationRect.setY((int)(m_requestedRegion.y()*m_exportedWindowRatio));
            m_destinationRect.setWidth((int)(m_requestedRegion.width()*m_exportedWindowRatio));
            m_destinationRect.setHeight((int)(m_requestedRegion.height()*m_exportedWindowRatio));
        }
        updateExportedItemSize();
    }
}

void WebOSExported::updateWindowState()
{
    if (!m_qwlsurfaceItem) {
        qWarning() << "QWaylandSurfaceItem for " << m_windowId << " is already destroyed";
        return;
    }
    WebOSSurfaceItem *item = qobject_cast<WebOSSurfaceItem*>(m_qwlsurfaceItem);
    qInfo() << "update window state : " << item->state() << "for WebOSExported ( " << m_windowId << " ) ";
    m_isSurfaceItemFullscreen = item->state() == Qt::WindowFullScreen;
    if (m_isSurfaceItemFullscreen) {
        calculateVideoDispRatio();
        calculateExportedItemRatio();
    }
}

void WebOSExported::updateVisible()
{
    if (!m_exportedItem) {
        qWarning() << "QWaylandSurfaceItem  for " << m_windowId << " is  already destroyed ";
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

    m_qwlsurfaceItem = nullptr;

    if (m_exportedItem) {
        delete m_exportedItem;
        m_exportedItem = nullptr;
    }
}

void WebOSExported::updateVideoWindowList(QString contextId, QRect videoDisplayRect, bool needRemove)
{
    if (needRemove || !m_exportedItem) {
        VideoWindowInformer::instance()->removeVideoWindowList(contextId);
    } else {
         if(!m_contextId.isNull() && m_exportedItem->isVisible()) {
            VideoWindowInformer::instance()->insertVideoWindowList(contextId, videoDisplayRect, m_windowId);
         }
    }
}

void WebOSExported::updateExportedItemSize()
{
    if (!m_exportedItem) {
        qWarning() << "QWaylandSurfaceItem  for " << m_windowId << " is  already destroyed ";
    }

    m_exportedItem->setX(m_destinationRect.x());
    m_exportedItem->setY(m_destinationRect.y());
    m_exportedItem->setWidth(m_destinationRect.width());
    m_exportedItem->setHeight(m_destinationRect.height());

    if (m_punchThroughItem) {
        qInfo()<< "punch through item size changed:  " << m_exportedItem->width() << "on " << m_windowId;
        m_punchThroughItem->setWidth(m_exportedItem->width());
        m_punchThroughItem->setHeight(m_exportedItem->height());
    }
    emit geometryChanged();
}

void WebOSExported::setVideoDisplayWindow()
{
    if (m_foreign->m_compositor->window() && !m_contextId.isNull()) {
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
        updateVideoWindowList(m_contextId, m_videoDisplayRect, false);
}

void WebOSExported::setDestinationRegion(struct::wl_resource *destination_region)
{
    if (destination_region) {
        const QRegion qwlDestinationRegion =
            QtWayland::Region::fromResource(
                destination_region)->region();
        if (qwlDestinationRegion.boundingRect().isValid()) {
            m_requestedRegion = QRect(
                qwlDestinationRegion.boundingRect().x(),
                qwlDestinationRegion.boundingRect().y(),
                qwlDestinationRegion.boundingRect().width(),
                qwlDestinationRegion.boundingRect().height());
        } else {
            m_requestedRegion = QRect(0, 0, 0, 0);
        }

        m_destinationRect = QRect(
            (int) (m_requestedRegion.x()*m_exportedWindowRatio),
            (int) (m_requestedRegion.y()*m_exportedWindowRatio),
            (int) (m_requestedRegion.width()*m_exportedWindowRatio),
            (int) (m_requestedRegion.height()*m_exportedWindowRatio));

        m_videoDisplayRect = QRect(
            (int) (m_requestedRegion.x()*m_videoDispRatio),
            (int) (m_requestedRegion.y()*m_videoDispRatio),
            (int) (m_requestedRegion.width()*m_videoDispRatio),
            (int) (m_requestedRegion.height()*m_videoDispRatio));

            qInfo() << "exported_window destination region : " << m_destinationRect << "on " << m_windowId;
            qInfo() << "video display output region : " <<m_videoDisplayRect << "on " << m_windowId;
    }

    setVideoDisplayWindow();
    updateExportedItemSize();
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
            } else {
                m_properties.insert("registerMute", "on");
            }
        }
    }

    if (m_foreign->m_compositor->window() && !m_contextId.isNull()) {
        VideoOutputdCommunicator::instance()->setProperty(name, value, m_contextId);
        if (value == "none") {
            m_properties.remove(name);
            return;
        }
    } else {
        qInfo() << "Do not call setProperty. Punch through is not working with " << m_windowId;
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
        if (m_punchThroughItem) {
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

void WebOSExported::assigneWindowId(QString windowId)
{
    m_windowId = windowId;
    qInfo() << m_windowId << "is assigned for " << this;
    send_window_id_assigned(m_windowId, m_exportedType);
}

QWaylandSurfaceItem *WebOSExported::getImportedItem()
{
    if (!m_exportedItem || m_exportedItem->childItems().isEmpty())
        return nullptr;

    if (m_exportedItem->childItems().size() > 1)
        qWarning() << "more than one imported item for  WebOSExported" << m_qwlsurfaceItem;

    // Imported surface item
    return static_cast<WebOSSurfaceItem *>(m_exportedItem->childItems().first());
}

void WebOSExported::setParentOf(QQuickItem *item)
{
    if (!m_qwlsurfaceItem || !m_exportedItem) {
        qWarning() << "unexpected null reference" << m_qwlsurfaceItem << m_exportedItem;
        return;
    }

    item->setParentItem(m_exportedItem);

    // mirrored items
    foreach(WebOSSurfaceItem *parent, m_qwlsurfaceItem->mirrorItems())
        startImportedMirroring(parent);
}

void WebOSExported::unregisterMuteOwner()
{
    if (!m_contextId.isNull()) {
         updateVideoWindowList(m_contextId, QRect(0, 0, 0, 0), true);
        QMap<QString, QString>::const_iterator it = m_properties.find("mute");
        if (it!= m_properties.end()) {
            if (it.value() == "on")
                VideoOutputdCommunicator::instance()->setProperty(it.key(), "off", m_contextId);
            m_properties.remove(it.key());
            VideoOutputdCommunicator::instance()->setProperty("registerMute", "off", m_contextId);
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

void WebOSExported::startImportedMirroring(QWaylandSurfaceItem *parent)
{
    if (!parent)
        return;

    QQuickItem *source = getImportedItem();
    // Nothing imported to the exported item
    if (!source)
        return;

    if (m_punchThroughItem) {
        PunchThroughMirrorItem *mirror = new PunchThroughMirrorItem();
        mirror->initialize(mirror, m_exportedItem, parent);
        mirror->setHandler(mirror, m_exportedItem, source);
    } else {
        QWaylandSurfaceItem *qsi = static_cast<QWaylandSurfaceItem *>(source);
        ImportedMirrorItem *mirror = new ImportedMirrorItem(static_cast<QWaylandQuickSurface *>(qsi->surface()));
        mirror->initialize(mirror, m_exportedItem, parent);
        mirror->setHandler(mirror, m_exportedItem, source);
    }
}

WebOSImported::WebOSImported(WebOSExported* exported,
                             struct wl_client* client, uint32_t id)
    : QtWaylandServer::wl_webos_imported(client, id,
                                         WEBOSIMPORTED_VERSION)
    , m_exported(exported)
{
    qInfo() << this << "is created. Video window id is : " << m_exported->m_windowId;
    connect(exported, &WebOSExported::geometryChanged,
            this, &WebOSImported::updateGeometry);
    if (exported)
        send_destination_region_changed(exported->m_destinationRect.width(),
                                        exported->m_destinationRect.height());
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
    if (m_childSurfaceItem && m_exported && m_exported->m_exportedItem) {
        qInfo() << "set surface item's width : " << m_exported->m_exportedItem->width();
        qInfo() << "set surface item's height : " << m_exported->m_exportedItem->height();

        m_childSurfaceItem->setWidth(m_exported->m_exportedItem->width());
        m_childSurfaceItem->setHeight(m_exported->m_exportedItem->height());
    }
}

void WebOSImported::updateGeometry()
{
    if (m_childSurfaceItem) {
        switch (m_textureAlign) {
        case WebOSImported::surface_alignment::surface_alignment_stretch:
            connect(m_childSurfaceItem->surface(), &QWaylandSurface::sizeChanged, this, &WebOSImported::setSurfaceItemSize);
            if (m_exported && m_exported->m_exportedItem) {
                m_childSurfaceItem->setWidth(m_exported->m_exportedItem->width());
                m_childSurfaceItem->setHeight(m_exported->m_exportedItem->height());
            }
            break;
        }
        //TODO: handle other cases.
    }

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
            webos_imported_detach_surface(nullptr, m_childSurfaceItem->surface()->handle()->resource()->handle);
        }
    }
}

void WebOSImported::updateExported(WebOSExported * exported)
{
    qInfo() << "WebOSImported::detach is called " << this;

    if (m_exported == exported)
        return;
    
    if (exported == nullptr)
        detach();

     m_exported = exported;
}

void WebOSImported::webos_imported_attach_punchthrough(Resource* r, const QString& contextId)
{
    Q_UNUSED(r);

    qInfo() << "attach_punchthrough is called with contextId " << contextId << " on " << this;

    if (!m_exported || !(m_exported->m_exportedItem) || contextId.isNull()) {
        qWarning() << " Fail to attach punch through (m_exported : " << m_exported << " contextId : " << contextId << ")";
        return;
    }
    if (!(m_exported->m_contextId.isNull()))
        qWarning() << "m_exported has m_contextId " << m_exported->m_contextId;

    QMap<QString, QString>::const_iterator it = (m_exported->m_properties).find("registerMute");
    if (it!= (m_exported->m_properties).end()) {
        VideoOutputdCommunicator::instance()->setProperty(it.key(), it.value(), contextId);
        m_exported->m_properties.remove(it.key());
    }
    if (!(m_exported->m_properties).isEmpty()) {
        QMap<QString, QString>::const_iterator i = m_exported->m_properties.constBegin();
        while (i != m_exported->m_properties.constEnd()) {
            qInfo() << "m_properties key : " << i.key() << " value :  " << i.value();
            VideoOutputdCommunicator::instance()->setProperty(i.key(), i.value(), contextId);
            ++i;
        }
    }
    m_exported->m_contextId = contextId;
    m_punchThroughAttached = true;

    if (m_exported->m_requestedRegion.isValid())
        m_exported->setVideoDisplayWindow();

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

    m_exported->unregisterMuteOwner();
    m_exported->setPunchThrough(false);
    send_punchthrough_detached(m_exported->m_contextId);
    m_exported->updateVideoWindowList(m_exported->m_contextId, QRect(0, 0, 0, 0), true);
    m_exported->m_contextId = QString::null;
    m_punchThroughAttached = false;
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

void WebOSImported::childSurfaceDestroyed()
{
    qInfo() << "childSurfaceDestroyed is called on " << this;
    if (m_childSurfaceItem)
        m_childSurfaceItem = nullptr;
}

void WebOSImported::webos_imported_attach_surface(
        Resource* resource,
        struct ::wl_resource* surface)
{
    qInfo() << "attach_surface is called : " << surface << "on " << this;

    if (!m_exported || !(m_exported->m_exportedItem) || !surface) {
        qWarning() << "Exported (" << m_exported << " ) is already destroyed or surface (" << surface << ") is null";
        return;
    }
    QtWayland::Surface* qwlSurface =
        QtWayland::Surface::fromResource(surface);
    QWaylandQuickSurface *quickSurface =
        qobject_cast<QWaylandQuickSurface *>(qwlSurface->waylandSurface());

    m_childSurfaceItem = quickSurface->surfaceItem();
    connect(m_childSurfaceItem->surface(), &QWaylandSurface::surfaceDestroyed, this, &WebOSImported::childSurfaceDestroyed);
    m_exported->setParentOf(m_childSurfaceItem);
    m_childSurfaceItem->setZ(m_exported->m_exportedItem->z()+m_z_index);
    updateGeometry();  //Resize texture if needed.

    send_surface_attached(surface);
}

void WebOSImported::webos_imported_detach_surface(
        Resource * resource,
        struct ::  wl_resource *surface)
{
    qInfo() <<"detach_surface is called : " << surface << " on " << this ;

    if (!m_childSurfaceItem || m_childSurfaceItem->surface()->handle()->resource()->handle != surface) {
        qWarning() << "surface is not the attached surface";
        return;
    }

    disconnect(m_childSurfaceItem->surface(), &QWaylandSurface::surfaceDestroyed, this, &WebOSImported::childSurfaceDestroyed);
    m_childSurfaceItem->setParentItem(nullptr);
    send_surface_detached(m_childSurfaceItem->surface()->handle()->resource()->handle);
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
