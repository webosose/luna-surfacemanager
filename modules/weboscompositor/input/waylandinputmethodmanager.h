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

#ifndef WAYLANDINPUTMETHODMANAGER_H
#define WAYLANDINPUTMETHODMANAGER_H

#include <QObject>
#include "weboscompositorexport.h"

class WaylandInputMethod;
/*!
 * Control is IME server started (Currently MaliitServer), and start it if it's needed.
 */
class WEBOS_COMPOSITOR_EXPORT WaylandInputMethodManager : public QObject
{
    Q_OBJECT
public:
    WaylandInputMethodManager(QObject* parent);
    ~WaylandInputMethodManager();
    virtual bool requestInputMethod();

public slots:
    void onInputMethodAvaliable(bool i_avaliable);
signals:
    void inputMethodAvaliable();

private:
    bool m_inputMethodAvaliable;
};

#endif //WAYLANDINPUTMETHODMANAGER_H
