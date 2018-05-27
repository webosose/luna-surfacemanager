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
    if (m_compositor) {
        AVOutputdCommunicator *pAVOutputdCommunicator = AVOutputdCommunicator::instance();
        if (pAVOutputdCommunicator) {
            WebOSCompositorWindow *pWebOSCompositorWindow = qobject_cast<WebOSCompositorWindow *>(compositor->window());
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
    m_exportedItem->setX(m_destinationRect.x());
    m_exportedItem->setY(m_destinationRect.y());
    m_exportedItem->setWidth(m_destinationRect.width());
    m_exportedItem->setHeight(m_destinationRect.height());
    m_exportedItem->setClip(true);

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

    AVOutputdCommunicator::instance()->setDisplayWindow(
        m_sourceRect, m_destinationRect, QString("MAIN"));

    m_exportedItem->setX(m_destinationRect.x());
    m_exportedItem->setY(m_destinationRect.y());
    m_exportedItem->setWidth(m_destinationRect.width());
    m_exportedItem->setHeight(m_destinationRect.height());
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
    foreach(WebOSImported* imported, m_importList) {
        delete imported;
    }

    detach();
    m_foreign->m_exportedList.removeAll(this);
    delete this;
}

WebOSImported::WebOSImported(WebOSExported* exported,
                             struct wl_client* client, uint32_t id)
    : QtWaylandServer::wl_webos_imported(client, id,
                                         WEBOSIMPORTED_VERSION)
    , m_exported(exported)
{
}

WebOSImported::~WebOSImported()
{
    if (m_punched)
        m_exported->detach();

    m_exported->m_importList.removeAll(this);

    foreach(Resource* resource, resourceMap().values()) {
        //When wl_exported deleted...
        wl_resource_destroy(resource->handle);
    }
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

    //TODO: Need method to adjust Z order.
    QtWayland::Surface* qwlSurface =
        QtWayland::Surface::fromResource(surface);
    if (surface) {
        QWaylandQuickSurface *quickSurface =
            qobject_cast<QWaylandQuickSurface *>(qwlSurface->waylandSurface());

        m_childSurface = quickSurface->surfaceItem();
        m_exported->setParentOf(m_childSurface);
    }
}
