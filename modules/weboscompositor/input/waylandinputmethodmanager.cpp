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

#include "waylandinputmethodmanager.h"
#include "waylandinputmethod.h"
#include <assert.h>
#include <QProcess>

WaylandInputMethodManager::WaylandInputMethodManager(QObject* parent)
    : QObject(parent)
    , m_inputMethodAvaliable(false)
{
}

WaylandInputMethodManager::~WaylandInputMethodManager()
{
}

bool WaylandInputMethodManager::requestInputMethod()
{
    if (m_inputMethodAvaliable)
        return true;

    QProcess::startDetached("/sbin/initctl", { "emit", "ime-activate" }, "./");
    return false;
}

void WaylandInputMethodManager::onInputMethodAvaliable(bool i_avaliable)
{
    if (m_inputMethodAvaliable == i_avaliable)
        return;

    m_inputMethodAvaliable = i_avaliable;
    if (i_avaliable)
        emit inputMethodAvaliable();
}
