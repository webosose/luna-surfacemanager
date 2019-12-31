// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#include "webosgroupedwindowmodel.h"

#include "webossurfaceitem.h"
#include "weboscompositortracer.h"

WebOSGroupedWindowModel::WebOSGroupedWindowModel()
    : WebOSWindowModel()
{
    PMTRACE_FUNCTION;
    connect(this, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)), this, SLOT(itemRemoved(const QModelIndex &, int, int)));
}

WebOSGroupedWindowModel::~WebOSGroupedWindowModel()
{
    PMTRACE_FUNCTION;
    int count = QSortFilterProxyModel::rowCount();
    for (int rowNumber = 0; rowNumber < count; rowNumber++) {
        WebOSSurfaceItem* item = get(rowNumber).value<WebOSSurfaceItem*>();
        if (item) {
            disconnect(item, SIGNAL(zOrderChanged(int)), this, SLOT(handleZOrderChange()));
            item->setGroupedWindowModel(0);
        }
    }
}

bool WebOSGroupedWindowModel::filterAcceptsRow(int sourceRow,
         const QModelIndex &sourceParent) const
{
    PMTRACE_FUNCTION;

    bool accepts = false;
    accepts = WebOSWindowModel::filterAcceptsRow(sourceRow, sourceParent);

    if (accepts) {
        QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
        WebOSSurfaceItem* item = sourceModel()->data(index0).value<WebOSSurfaceItem*>();
        if (item) {
            bool isconnected = connect(item, SIGNAL(zOrderChanged(int)), this, SLOT(handleZOrderChange()));
            if (isconnected) {
                item->setGroupedWindowModel((WebOSGroupedWindowModel*)this);
            }
        }
    }

    return accepts;
}

void WebOSGroupedWindowModel::handleZOrderChange() {
    m_filterDirty = true;
    emit deferredInvalidate();
}

void WebOSGroupedWindowModel::itemRemoved(const QModelIndex& parent, int start, int end)
{
    for (int row = start; row <= end; row++) {
        WebOSSurfaceItem* item = data(index(row, 0, parent)).value<WebOSSurfaceItem*>();
        if (item) {
            disconnect(item, SIGNAL(zOrderChanged(int)), this, SLOT(handleZOrderChange()));
            item->setGroupedWindowModel(0);
        }
    }
}
