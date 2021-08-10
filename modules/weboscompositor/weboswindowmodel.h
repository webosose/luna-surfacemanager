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

#ifndef WEBOSWINDOWMODEL_H
#define WEBOSWINDOWMODEL_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QList>
#include <QHash>

class WebOSSurfaceModel;
class WebOSSurfaceItem;
class QQmlComponent;

class WEBOS_COMPOSITOR_EXPORT WebOSWindowModel : public QSortFilterProxyModel {
    Q_OBJECT

    Q_PROPERTY(QString windowType READ windowType WRITE setWindowType NOTIFY windowTypeChanged);
    Q_PROPERTY(WebOSSurfaceModel* surfaceSource READ surfaceSource WRITE setSurfaceSource NOTIFY surfaceSourceChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    Q_MOC_INCLUDE("webossurfacemodel.h")
#endif
    Q_PROPERTY(QString sortFunction READ sortFunction WRITE setSortFunction NOTIFY sortFunctionChanged);
    Q_PROPERTY(QString acceptFunction READ acceptFunction WRITE setAcceptFunction NOTIFY acceptFunctionChanged);
    Q_PROPERTY(int count READ count NOTIFY countChanged);
    Q_PROPERTY(bool locked READ locked WRITE setLocked NOTIFY lockedChanged);

public:
    WebOSWindowModel();
    ~WebOSWindowModel();

    QString windowType() const;
    void setWindowType(QString type);

    WebOSSurfaceModel* surfaceSource() const;
    void setSurfaceSource(WebOSSurfaceModel* source);

    QString sortFunction() const;
    void setSortFunction(QString func);

    QString acceptFunction() const;
    void setAcceptFunction(QString func);

    Q_INVOKABLE QVariant get(int row);
    int count();

    Q_INVOKABLE int indexForItem(QVariant);

    bool hasChildren (const QModelIndex& ) const override { return false; }
    QModelIndex parent(const QModelIndex& ) const override { return QModelIndex(); }

    bool locked();
    void setLocked(bool);

signals:
    void windowTypeChanged();
    void surfaceSourceChanged();
    void sortFunctionChanged();
    void acceptFunctionChanged();
    void surfaceAdded(WebOSSurfaceItem* item);
    void surfaceRemoved(WebOSSurfaceItem* item);
    void surfaceRemovalFinished();
    void countChanged();
    void lockedChanged();
    void deferredInvalidate();
    void invalidated();

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    void deferInvalidate();

protected slots:
    void emitSurfacesRemoved(const QModelIndex & parent, int start, int end);
    void emitSurfacesRemovalFinished(const QModelIndex & parent, int start, int end);
    void emitSurfacesAdded(const QModelIndex & parent, int start, int end);
    virtual void handleInvalidate();

protected:
    bool m_filterDirty;

private:
    QString m_type;
    QString m_sortFunc;
    QString m_acceptFunc;
    bool m_locked;
};

#endif
