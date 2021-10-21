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

#ifndef WEBOSFOREIGN_H
#define WEBOSFOREIGN_H

#include <QObject>
#include <QQuickItem>
#include <QtWaylandCompositor/private/qwayland-server-wayland.h>
#include <WebOSCoreCompositor/private/qwayland-server-webos-foreign.h>
#include <WebOSCoreCompositor/weboscompositorexport.h>
#include <QtWaylandCompositor/private/qwaylandsurface_p.h>
#include <QtWaylandCompositor/private/qwlregion_p.h>
#include <QPointer>

#include "webossurfaceitem.h"

#define WEBOSFOREIGN_VERSION 1
#define WEBOSEXPORTED_VERSION 2
#define WEBOSIMPORTED_VERSION 2

class WebOSCoreCompositor;
class WebOSCompositorWindow;
class WebOSExported;
class WebOSImported;

class WEBOS_COMPOSITOR_EXPORT WebOSForeign : public QObject, public QtWaylandServer::wl_webos_foreign
{
    Q_OBJECT
public:
    enum WebOSExportedType {
        VideoObject    = 0,    // Exported object is Video
        SubtitleObject = 1,     // Exported object is Subtitle
        TransparentObject = 2,    // Exported object is Transparent
        OpaqueObject = 3           // Exported object is Opaque. It always needs punch through.
    };

    WebOSForeign(WebOSCoreCompositor* compositor);
    void registeredWindow();

protected:
    virtual WebOSExported* createExported(struct wl_client* client,
                                          uint32_t id, WebOSSurfaceItem* surfaceItem,
                                          WebOSExportedType exportedType);

    virtual WebOSImported* createImported(WebOSExported *exported, struct wl_client *client,
                                          uint32_t id, WebOSExportedType exportedType);

    virtual void webos_foreign_export_element(Resource *resource,
                                              uint32_t id,
                                              struct ::wl_resource *surface,
                                              uint32_t exported_type) override;
    virtual void webos_foreign_import_element(Resource *resource,
                                              uint32_t id,
                                              const QString &window_id,
                                              uint32_t exported_type) override;

    WebOSCoreCompositor* m_compositor = nullptr;
    QList<WebOSExported*> m_exportedList;

    friend class WebOSExported;
    friend class WebOSImported;
};

class WEBOS_COMPOSITOR_EXPORT WebOSExported : public QObject, public QtWaylandServer::wl_webos_exported
{
    Q_OBJECT
public:
    enum ExportState {
        NoState  = 0,
        Punched  = 2,
        Textured = 4
    };
    Q_DECLARE_FLAGS(ExportStates, ExportState)

    WebOSExported(WebOSForeign* foreign, struct wl_client* client,
                  uint32_t id, WebOSSurfaceItem* surfaceItem,
                  WebOSForeign::WebOSExportedType exportedType);
    ~WebOSExported();
    void setDestinationRegion(struct::wl_resource *destination_region);
    void setPunchThrough(bool needPunch);
    void assignWindowId(QString windowId);
    void setParentOf(QQuickItem *surfaceItem);
    void updateCoverState();
    void updateActiveRegion();
    void registerMuteOwner(const QString& contextId);
    void unregisterMuteOwner();
    void setVideoDisplayWindow();
    void startImportedMirroring(WebOSSurfaceItem *parent);
    bool hasSecuredContent();
    void updateDestinationRegionByActiveRegion();
    void setDestinationRect();
    void setVideoDisplayRect();

    WebOSSurfaceItem *surfaceItem() const { return m_surfaceItem; }

signals:
    void geometryChanged();

protected:
    virtual void webos_exported_destroy(Resource *) override;
    virtual void webos_exported_destroy_resource(Resource *) override;
    virtual void webos_exported_set_exported_window(Resource *resource,
                                  struct ::wl_resource *source_region,
                                  struct ::wl_resource *destination_region) override;
    virtual void webos_exported_set_crop_region(Resource *resource,
                                  struct ::wl_resource *original_input,
                                  struct ::wl_resource *source_region,
                                  struct ::wl_resource *destination_region) override;
    virtual void webos_exported_set_property(Resource *resource,
                                  const QString &name,
                                  const QString &value) override;

private slots:
    void updateVisible();
    void updateWindowState();
    void calculateAll();
    void onSurfaceDestroyed();
    void updateCompositorWindow(QQuickWindow *window);

protected:
    void updateVideoWindowList(QString contextId, QRect videoDisplayRect, bool needRemove);
    void calculateVideoDispRatio();
    void calculateExportedItemRatio();
    void updateExportedItemSize();
    WebOSSurfaceItem *getImportedItem();

    WebOSForeign* m_foreign = nullptr;
    WebOSCompositorWindow* m_compositorWindow = nullptr;
    QPointer<WebOSSurfaceItem> m_surfaceItem;
    QPointer<QQuickItem> m_exportedItem;
    QPointer<QQuickItem> m_punchThroughItem;
    QList<WebOSImported*> m_importList;
    WebOSForeign::WebOSExportedType m_exportedType = WebOSForeign::VideoObject;
    QRect m_originalInputRect;
    QRect m_sourceRect;
    QRect m_destinationRect;
    QRect m_originalRequestedRegion;
    QRect m_requestedRegion;
    QRect m_videoDisplayRect;
    QString m_windowId;
    QString m_contextId;
    QString m_muteRegisteredContextId;
    QMap<QString, QString> m_properties;
    bool m_isSurfaceItemFullscreen;
    bool m_directVideoScalingMode = false; // If this mode is enabled, do not call setDisplayWindow and setCropRegion of videooutputd
    bool m_coverVideo;
    QRect m_activeRegion;
    double m_videoDispRatio = 1.0;
    double m_exportedWindowRatio = 1.0;

    friend class WebOSForeign;
    friend class WebOSImported;
};

class WEBOS_COMPOSITOR_EXPORT WebOSImported :
                             public QObject,
                             public QtWaylandServer::wl_webos_imported
{
public:
    WebOSImported() = delete;
    ~WebOSImported();
    void destroyChildDisplay();
    void childSurfaceDestroyed();
    void destroyResource();
    void detach();
    void setSurfaceItemSize();
    void updateExported(WebOSExported * exported);
    void updateGeometry();

protected:
    virtual void webos_imported_attach_punchthrough(Resource *) override;
    virtual void webos_imported_attach_punchthrough_with_context(Resource *,
                                                 const QString& contextId) override;
    virtual void webos_imported_detach_punchthrough(Resource *) override;
    virtual void webos_imported_destroy(Resource*) override;
    virtual void webos_imported_destroy_resource(Resource *) override;
    virtual void webos_imported_attach_surface(Resource *resource,
                                               struct ::wl_resource *surface) override;
    virtual void webos_imported_detach_surface(Resource * resource,
                                                struct ::  wl_resource *surface) override;
    virtual void webos_imported_set_z_index(Resource *surface,
                                                 int32_t z_index) override;
    virtual void webos_imported_set_surface_alignment(Resource * resource,
                                                uint32_t surface_alignment) override;

protected:
    WebOSImported(WebOSExported* exported, struct wl_client* client, uint32_t id,
                        WebOSForeign::WebOSExportedType exportedType);
    WebOSExported* m_exported = nullptr;
    WebOSSurfaceItem* m_childSurfaceItem = nullptr;
    QQuickItem* m_childDisplayItem = nullptr;
    enum surface_alignment m_textureAlign = surface_alignment::surface_alignment_stretch;
    int32_t m_z_index = 0;
    bool m_punchThroughAttached = false;
    QString m_contextId;
    WebOSForeign::WebOSExportedType m_importedType;

    friend class WebOSForeign;
};

#endif
