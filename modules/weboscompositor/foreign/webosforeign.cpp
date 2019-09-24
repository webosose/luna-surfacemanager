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

#include "avoutputd_communicator.h"
#include "punchthroughelement.h"
#include "weboscorecompositor.h"
#include "weboscompositorwindow.h"
#include "webosforeign.h"

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

WebOSForeign::WebOSForeign(WebOSCoreCompositor* compositor)
    : QtWaylandServer::wl_webos_foreign(compositor->waylandDisplay(), WEBOSFOREIGN_VERSION)
    , m_compositor(compositor)
{
}


void WebOSForeign::registeredWindow()
{
    if (m_compositor) {
        AVOutputdCommunicator *pAVOutputdCommunicator = AVOutputdCommunicator::instance();
        if (pAVOutputdCommunicator) {
            WebOSCompositorWindow *pWebOSCompositorWindow = qobject_cast<WebOSCompositorWindow *>(m_compositor->window());
            if (pWebOSCompositorWindow)
                pWebOSCompositorWindow->rootContext()->setContextProperty(QLatin1String("avoutputdCommunicator"), pAVOutputdCommunicator);
            else
                qFatal("Failed to get the window instance");
        } else {
            qFatal("Failed to get AVOutputdCommunicator instance");
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
    foreach(WebOSExported* exported, m_exportedList) {
        if (exported->m_windowId == window_id &&
            exported->m_exportedType == exported_type) {
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
               << window_id << ", " << exported_type;
}

WebOSExported::WebOSExported(
        WebOSForeign* foreign,
        struct wl_client* client,
        uint32_t id,
        QWaylandSurfaceItem* surfaceItem,
        WebOSForeign::WebOSExportedType exportedType)
    : QtWaylandServer::wl_webos_exported(client, id, WEBOSEXPORTED_VERSION)
    , m_foreign(foreign)
    , m_qwlsurfaceItem(surfaceItem)
    , m_exportedItem(new QQuickItem(surfaceItem))
    , m_exportedType(exportedType)
{
    m_exportedItem->setClip(true);
    m_exportedItem->setZ(-1);
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
    while (!m_importList.isEmpty())
        m_importList.takeFirst()->destroyResource();

    // delete m_punchThroughItem in detach
    detach();
    m_foreign->m_exportedList.removeAll(this);

    // Usually it is deleted by QObjectPrivate::deleteChildren with its parent surfaceItem.
    if (m_exportedItem)
        delete m_exportedItem;
}

void WebOSExported::webos_exported_set_exported_window(
        Resource *resource,
        struct ::wl_resource *source_region,
        struct ::wl_resource *destination_region)
{
    if (source_region) {
        const QRegion qwlSourceRegion =
            QtWayland::Region::fromResource(source_region)->region();

        if (qwlSourceRegion.boundingRect().isValid()) {
            m_sourceRect = QRect(
                qwlSourceRegion.boundingRect().x(),
                qwlSourceRegion.boundingRect().y(),
                qwlSourceRegion.boundingRect().width(),
                qwlSourceRegion.boundingRect().height());
        }
    }

    if (destination_region) {
        const QRegion qwlDestinationRegion =
            QtWayland::Region::fromResource(
                destination_region)->region();
        if (qwlDestinationRegion.boundingRect().isValid()) {
            m_destinationRect = QRect(
                qwlDestinationRegion.boundingRect().x(),
                qwlDestinationRegion.boundingRect().y(),
                qwlDestinationRegion.boundingRect().width(),
                qwlDestinationRegion.boundingRect().height());
        }
    }

    if (m_foreign->m_compositor->window()) {
        AVOutputdCommunicator::instance()->setDisplayWindow(
            m_sourceRect, m_destinationRect, QString("MAIN"));
    }

    m_exportedItem->setX(m_destinationRect.x());
    m_exportedItem->setY(m_destinationRect.y());
    m_exportedItem->setWidth(m_destinationRect.width());
    m_exportedItem->setHeight(m_destinationRect.height());

    if (m_punchThroughItem) {
        m_punchThroughItem->setWidth(m_exportedItem->width());
        m_punchThroughItem->setHeight(m_exportedItem->height());
    }

    emit geometryChanged();
}

void WebOSExported::setPunchTrough()
{
    if (!m_punchThroughItem) {
        PunchThroughItem* punchThroughNativeItem =
            new PunchThroughItem();
        punchThroughNativeItem->setParentItem(m_exportedItem);
        punchThroughNativeItem->setWidth(m_destinationRect.width());
        punchThroughNativeItem->setHeight(m_destinationRect.height());
        m_punchThroughItem = punchThroughNativeItem;
    }
}

void WebOSExported::detach()
{
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

void WebOSExported::assigneWindowId(QString windowId)
{
    m_windowId = windowId;
    send_window_id_assigned(m_windowId, m_exportedType);
}

void WebOSExported::setParentOf(QWaylandSurfaceItem* surfaceItem)
{
    //TODO: Need location settings? and Z order manage
    surfaceItem->setParentItem(m_exportedItem);
}

void WebOSExported::webos_exported_destroy_resource(Resource *r)
{
    Q_ASSERT(resourceMap().size() == 0);
    delete this;
}

WebOSImported::WebOSImported(WebOSExported* exported,
                             struct wl_client* client, uint32_t id)
    : QtWaylandServer::wl_webos_imported(client, id,
                                         WEBOSIMPORTED_VERSION)
    , m_exported(exported)
{
    connect(exported, &WebOSExported::geometryChanged,
            this, &WebOSImported::updateGeometry);
    if (exported && exported->m_destinationRect.isValid())
        send_destination_region_changed(exported->m_destinationRect.width(),
                                        exported->m_destinationRect.height());
}

WebOSImported::~WebOSImported()
{
    if (!m_exported)
        return;

    if (m_punched)
        m_exported->detach();

    m_exported->m_importList.removeAll(this);
}

void WebOSImported::destroyResource()
{
    // When wl_exported deleted...
    foreach(Resource* resource, resourceMap().values()) {
        wl_resource_destroy(resource->handle);
    }
}

void WebOSImported::updateGeometry()
{
    if (m_childSurface) {
        switch (m_textureAlign) {
        case WebOSImported::surface_alignment::surface_alignment_stretch:
            disconnect(m_childSurface->surface(), &QWaylandSurface::sizeChanged, 0, 0);
            m_childSurface->setWidth(m_exported->m_exportedItem->width());
            m_childSurface->setHeight(m_exported->m_exportedItem->height());
            break;
        }
        //TODO: handle other cases.
    }

    send_destination_region_changed(m_exported->m_exportedItem->width(),
                                    m_exported->m_exportedItem->height());
}

void WebOSImported::webos_imported_attach_punchthrough(Resource* r)
{
    Q_UNUSED(r);
    if (!m_punched) {
        m_exported->setPunchTrough();
        m_punched = true;
    }
}

void WebOSImported::webos_imported_destroy_resource(Resource* r)
{
    Q_UNUSED(r);

    if (resourceMap().isEmpty())
        delete this;
}

void WebOSImported::webos_imported_attach_surface(
        Resource* resource,
        struct ::wl_resource* surface)
{
    if (m_punched) {
        qWarning() << "Imported can only have one state, punch-trough or texture";
        return;
    }

    if (surface) {
        //TODO: Need method to adjust Z order.
        QtWayland::Surface* qwlSurface =
            QtWayland::Surface::fromResource(surface);
        QWaylandQuickSurface *quickSurface =
            qobject_cast<QWaylandQuickSurface *>(qwlSurface->waylandSurface());

        m_childSurface = quickSurface->surfaceItem();
        // NOTE: Keep m_childSurface visible, which makes that m_childSurface
        // is de-parented from m_exportedItem on destroying m_exportedItem.
        // If m_childSurface is not visible, the parent, m_exportedItem deletes m_childSurface
        // on the parent's destruction. That might cause a double free.
        // Refer to ~QQuickItem() and QObjectPrivate::deleteChildren().
        m_exported->setParentOf(m_childSurface);
        updateGeometry();  //Resize texture if needed.
    }
}
