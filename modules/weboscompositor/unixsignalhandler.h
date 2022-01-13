// Copyright (c) 2016-2022 LG Electronics, Inc.
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

#pragma once

#include <QObject>
#include <QSocketNotifier>

#include "weboscompositorexport.h"

class WEBOS_COMPOSITOR_EXPORT UnixSignalHandler : public QObject
{
    Q_OBJECT

public:
    UnixSignalHandler(QObject*);
    ~UnixSignalHandler();

    // Unix signal handlers.
    static void deliverUnixSignaltoQt(int sigNo);

public slots:
    void handleSignal();

signals:
    void sighup();

private:
    static int m_sigFd[2];

    QSocketNotifier *m_signalNotifier;
};
