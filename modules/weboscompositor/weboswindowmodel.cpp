// Copyright (c) 2013-2018 LG Electronics, Inc.
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

#include "weboswindowmodel.h"
#include "webossurfacemodel.h"
#include "webossurfaceitem.h"
#include "weboscompositortracer.h"

WebOSWindowModel::WebOSWindowModel()
    : m_locked(false),
      m_filterDirty(false)
{
    PMTRACE_FUNCTION;
    setDynamicSortFilter(true);
    QSortFilterProxyModel::sort(0);
    connect(this, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)), this, SLOT(emitSurfacesRemoved(const QModelIndex &, int, int)));
    connect(this, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SLOT(emitSurfacesRemovalFinished(const QModelIndex &, int, int)));
    connect(this, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(emitSurfacesAdded(const QModelIndex&, int, int)));

    connect(this, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SIGNAL(countChanged()));
    connect(this, SIGNAL(deferredInvalidate()), this, SLOT(handleInvalidate()), Qt::QueuedConnection);
}

void WebOSWindowModel::deferInvalidate() {
    if(m_filterDirty) {
        return;
    } else {
        m_filterDirty = true;
        emit deferredInvalidate();
    }
}

void WebOSWindowModel::handleInvalidate() {
    if(m_filterDirty) {// might have been canceled by a source update
        invalidate();
        emit invalidated();
    }
    m_filterDirty = false;
}

WebOSWindowModel::~WebOSWindowModel() {
}

QString WebOSWindowModel::windowType() const {
    return m_type;
}

void WebOSWindowModel::setWindowType(QString type) {
    PMTRACE_FUNCTION;
    if (type != m_type) {
        m_type = type;
        deferInvalidate();
        emit windowTypeChanged();
    }
}

QString WebOSWindowModel::sortFunction() const {
    return m_sortFunc;
}

void WebOSWindowModel::setSortFunction(QString sortFunc) {
    PMTRACE_FUNCTION;
    if (sortFunc != m_sortFunc) {
        m_sortFunc = sortFunc;
        deferInvalidate();
        emit sortFunctionChanged();
    }
}

QString WebOSWindowModel::acceptFunction() const {
    return m_acceptFunc;
}

void WebOSWindowModel::setAcceptFunction(QString func) {
    PMTRACE_FUNCTION;
    if (func != m_acceptFunc) {
        m_acceptFunc = func;
        deferInvalidate();
        emit acceptFunctionChanged();
    }
}

bool WebOSWindowModel::lessThan(const QModelIndex &left,
                                const QModelIndex &right) const
{
    PMTRACE_FUNCTION;
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);
    QVariant returnedValue;
    QMetaObject::invokeMethod(const_cast<WebOSWindowModel*>(this), m_sortFunc.toUtf8().constData(),
        Q_RETURN_ARG(QVariant, returnedValue),
        Q_ARG(QVariant, leftData),
        Q_ARG(QVariant, rightData)
        );
    return returnedValue.toBool();
}

QVariant WebOSWindowModel::get(int row)
{
    PMTRACE_FUNCTION;
    QModelIndex idx = QSortFilterProxyModel::index(row, 0);
    return idx.data();
}

bool WebOSWindowModel::filterAcceptsRow(int sourceRow,
         const QModelIndex &sourceParent) const
{
    PMTRACE_FUNCTION;
    // no filters defined, accept all window types
    if(windowType().isEmpty() && m_acceptFunc.isEmpty()) {
        return true;
    }

    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    WebOSSurfaceItem* item = sourceModel()->data(index0).value<WebOSSurfaceItem*>();
    bool accepts = false;
    if (!m_acceptFunc.isEmpty()) {
        QVariant returnedValue;
        QMetaObject::invokeMethod(const_cast<WebOSWindowModel*>(this), m_acceptFunc.toUtf8().constData(),
                Q_RETURN_ARG(QVariant, returnedValue),
                Q_ARG(QVariant, QVariant::fromValue(item)));
        accepts = returnedValue.toBool();
    } else {
        accepts = item->type() == windowType();
    }
    return accepts;
}


WebOSSurfaceModel* WebOSWindowModel::surfaceSource() const {
    return static_cast<WebOSSurfaceModel*>(sourceModel());
}

void WebOSWindowModel::setSurfaceSource(WebOSSurfaceModel* source) {
    PMTRACE_FUNCTION;
    if (source != sourceModel()) {
        // changing the source also invalidates the filter already,
        // so we don't have to invalidate twice
        m_filterDirty = false;
        setSourceModel(source);
        emit surfaceSourceChanged();
    }
}

// these indices are inclusive
void WebOSWindowModel::emitSurfacesRemoved(const QModelIndex& parent, int start, int end)
{
    PMTRACE_FUNCTION;
    for (int row = start; row <= end; row++) {
        WebOSSurfaceItem* item = data(index(row, 0, parent)).value<WebOSSurfaceItem*>();
        emit const_cast<WebOSWindowModel*>(this)->surfaceRemoved(item);
    }
}

void WebOSWindowModel::emitSurfacesRemovalFinished(const QModelIndex& parent, int start, int end)
{
    PMTRACE_FUNCTION;
    emit surfaceRemovalFinished();
}

// these indices are inclusive
void WebOSWindowModel::emitSurfacesAdded(const QModelIndex& parent, int start, int end){
    PMTRACE_FUNCTION;
    for(int row = start; row <= end; row++) {
        WebOSSurfaceItem* item = data(index(row, 0, parent)).value<WebOSSurfaceItem*>();
        emit const_cast<WebOSWindowModel*>(this)->surfaceAdded(item);
    }
}

int WebOSWindowModel::count()
{
    return rowCount();
}

int WebOSWindowModel::indexForItem(QVariant value)
{
    int val = -1;
    for (int i = 0; i < count(); i++) {
        QModelIndex idx = this->index(i, 0);
        if (idx.data() == value) {
           val = idx.row();
           break;
        }
    }
    return val;
}

bool WebOSWindowModel::locked()
{
    return m_locked;
}

void WebOSWindowModel::setLocked(bool locked)
{
    if (m_locked == locked)
        return;

    m_locked = locked;
    setDynamicSortFilter(!m_locked);
    // After set the dynamicSortFilter, the model should be filtered again.
    if (!m_locked)
        invalidateFilter();

    emit lockedChanged();
}
