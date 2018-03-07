// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QDebug>

#include "unixsignalhandler.h"

int UnixSignalHandler::m_sigFd[2];

UnixSignalHandler::UnixSignalHandler(QObject* parent)
    : QObject(parent)
    , m_signalNotifier(0)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigFd)) {
        qWarning() << "Can't create socket for unix signal handling.";
        return;
    }

    m_signalNotifier = new QSocketNotifier(m_sigFd[1], QSocketNotifier::Read, this);
    connect(m_signalNotifier, &QSocketNotifier::activated, this, &UnixSignalHandler::handleSignal);

    struct sigaction hup;

    hup.sa_handler = UnixSignalHandler::deliverUnixSignaltoQt;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = SA_RESTART;

    if (sigaction(SIGHUP, &hup, 0) != 0)
        qWarning() << "Can't register unix signal handler.";
}

UnixSignalHandler::~UnixSignalHandler()
{
    if (m_signalNotifier)
        delete m_signalNotifier;
    close(m_sigFd[0]);
    close(m_sigFd[1]);
}

void UnixSignalHandler::deliverUnixSignaltoQt(int sigNo)
{
    ::write(m_sigFd[0], &sigNo, sizeof(sigNo));
}

void UnixSignalHandler::handleSignal()
{
    if (!m_signalNotifier)
        return;

    int sigNo;

    m_signalNotifier->setEnabled(false);
    if (::read(m_sigFd[1], &sigNo, sizeof(sigNo)) > 0) {
        switch (sigNo) {
        case SIGHUP:
            emit sighup();
            break;
        }
    }

    m_signalNotifier->setEnabled(true);
    qDebug() << "SIGNAL " << sigNo << " handled.";
}
