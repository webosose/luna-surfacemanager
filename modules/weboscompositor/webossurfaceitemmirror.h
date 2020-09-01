// Copyright (c) 2020 LG Electronics, Inc.
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

#ifndef WEBOSSURFACEITEMMIRROR_H
#define WEBOSSURFACEITEMMIRROR_H

#include <QObject>
#include <QQuickItem>

class WebOSSurfaceItem;

class WebOSSurfaceItemMirror : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(WebOSSurfaceItem *sourceItem READ sourceItem WRITE setSourceItem NOTIFY sourceItemChanged)
    Q_PROPERTY(bool clustered READ clustered WRITE setClustered NOTIFY clusteredChanged)
    Q_PROPERTY(bool propagateEvents READ propagateEvents WRITE setPropagateEvents NOTIFY propagateEventsChanged)

public:
    WebOSSurfaceItemMirror();
    ~WebOSSurfaceItemMirror();

    WebOSSurfaceItem *sourceItem() { return m_sourceItem; }
    void setSourceItem(WebOSSurfaceItem *sourceItem);

    bool clustered() { return m_clustered; }
    void setClustered(bool clustered);

    bool propagateEvents() { return m_propagateEvents; }
    void setPropagateEvents(bool propagateEvents);

signals:
    void sourceItemChanged();
    void clusteredChanged();
    void propagateEventsChanged();

protected:
    virtual void hoverMoveEvent(QHoverEvent *event) override;
    virtual void hoverEnterEvent(QHoverEvent *event) override;
    virtual void hoverLeaveEvent(QHoverEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void touchEvent(QTouchEvent *event) override;

private:
    bool needToPropagate(QEvent *event);
    QPointF translatePoint(QPointF point);

    WebOSSurfaceItem *m_mirrorItem = nullptr;
    WebOSSurfaceItem *m_sourceItem = nullptr;
    bool m_clustered = false;
    bool m_propagateEvents = false;

    QMetaObject::Connection m_widthChangedConnection;
    QMetaObject::Connection m_heightChangedConnection;
    QMetaObject::Connection m_sourceDestroyedConnection;
    QMetaObject::Connection m_mirrorDestroyedConnection;
};

#endif // WEBOSSURFACEITEMMIRROR_H
