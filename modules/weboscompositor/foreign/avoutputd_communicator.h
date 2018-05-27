// Copyright (c) 2018-2019 LG Electronics, Inc.
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

#ifndef AVOUTPUTD_COMMUNICATOR_H
#define AVOUTPUTD_COMMUNICATOR_H

#include <QObject>
#include <QRect>
#include <QString>

class AVOutputdCommunicator : public QObject
{
    Q_OBJECT
public:
    static AVOutputdCommunicator* instance();

    void setDisplayWindow(QRect sourceRectangle, QRect destinationRectangle, QString sink = QString("MAIN"));

signals:
    void setVideoDisplayWindowRequested(const QRect sourceRectangle, const QRect destinationRectangle, const QString sink);

protected:
    AVOutputdCommunicator(QObject *parent = Q_NULLPTR);

private:
    static AVOutputdCommunicator* m_instance;
};

#endif  //#ifndef AVOUTPUTD_COMMUNICATOR_H
