// Copyright (c) 2013-2022 LG Electronics, Inc.
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

#ifndef WAYLANDINPUTMETHOD_H
#define WAYLANDINPUTMETHOD_H

#include <QObject>
#include <QRect>
#include <QVector>
#include <QPointer>

#include <wayland-server.h>
#include <wayland-input-method-server-protocol.h>

#include "webossurfaceitem.h"
#include "waylandinputpanel.h"

class QWaylandCompositor;
class QWaylandSeat;
class WaylandTextModel;
class WaylandInputMethodContext;
class WaylandInputMethodManager;
class WebOSSurfaceItem;

/*!
 * Talks with the input method sitting in the VKB
 * process.
 */
class WEBOS_COMPOSITOR_EXPORT WaylandInputMethod : public QObject {

    Q_OBJECT

    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(bool allowed READ allowed WRITE setAllowed NOTIFY allowedChanged)

    Q_PROPERTY(QRect panelRect READ panelRect WRITE setPanelRect NOTIFY panelRectChanged)
    Q_PROPERTY(QRect preferredPanelRect READ preferredPanelRect NOTIFY preferredPanelRectChanged)
    Q_PROPERTY(bool hasPreferredPanelRect READ hasPreferredPanelRect NOTIFY hasPreferredPanelRectChanged)
    Q_PROPERTY(WebOSSurfaceItem* targetSurfaceItem READ targetSurfaceItem NOTIFY setTargetSurfaceItemChanged)

public:
    WaylandInputMethod();
    WaylandInputMethod(QWaylandCompositor* compositor);
    ~WaylandInputMethod();

    virtual void initialize();

    static void destroyInputMethod(struct wl_resource* resource);
    static void setDisplayId(struct wl_client *client, struct wl_resource *resource, uint32_t id);

    void binding(struct wl_client *client, uint32_t id, wl_resource_destroy_func_t destroy = destroyInputMethod);
    void unbinding();
    virtual void handleDestroy();
    wl_resource* handle() const { return m_resource; }
    void setHandle(wl_resource*handle) { m_resource = handle; }
    wl_client* client() const { return m_client; }
    void setClient(wl_client *client) { m_client = client; }
    QWaylandCompositor* compositor() const { return m_compositor; }
    WaylandInputPanel* inputPanel() const { return m_inputPanel; }
    void setInputPanel(WaylandInputPanel *panel);
    WaylandInputMethodManager* inputMethodManager() const { return m_inputMethodManager; }
    Q_INVOKABLE bool active() const { return m_activeContext != NULL; }
    WaylandInputMethodContext* activeContext() { return m_activeContext; }
    bool isActiveModel(WaylandTextModel *model) const;
    bool allowed() const { return m_allowed; };
    void setAllowed(bool allowed);

    QRect panelRect() const;
    void setPanelRect(const QRect& rect);

    QRect preferredPanelRect() const { return m_preferredPanelRect; }
    void setPreferredPanelRect(const QRect& rect);
    void resetPreferredPanelRect();
    bool hasPreferredPanelRect() const { return m_hasPreferredPanelRect; }
    void setTargetSurfaceItem(WebOSSurfaceItem *item);
    WebOSSurfaceItem* targetSurfaceItem() const { return m_targetSurfaceItem; }
    int displayId() { return m_displayId; }

    QWaylandSeat *inputDevice();

public slots:
    void deactivate();
    void contextActivated();
    void contextDeactivated();

signals:
    void inputMethodBound(bool);
    void activeChanged();
    void allowedChanged();
    void panelRectChanged();
    void preferredPanelRectChanged();
    void hasPreferredPanelRectChanged();
    void setTargetSurfaceItemChanged();
    void displayIdChanged();

protected:
    static const struct input_method_interface inputMethodImplementation;

private:
    void setHasPreferredPanelRect(const bool flag);
    void setDisplayId(const uint32_t displayId);

    QWaylandCompositor* m_compositor;
    struct wl_resource* m_resource;
    struct wl_client* m_client;

    WaylandInputMethodContext* m_activeContext;
    QPointer<WaylandInputPanel> m_inputPanel;
    QRect m_preferredPanelRect;
    bool m_hasPreferredPanelRect;
    WebOSSurfaceItem* m_targetSurfaceItem;
    WaylandInputMethodManager* m_inputMethodManager;
    bool m_allowed;
    int m_displayId;
};

#endif //WAYLANDINPUTMETHOD_H
