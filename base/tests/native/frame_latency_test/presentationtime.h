// Copyright (c) 2021 LG Electronics, Inc.
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

#include <QObject>
#include <webosplatform.h>
#include <webospresentationtime.h>

class PresentationTime : public QObject
{
Q_OBJECT
public:
    PresentationTime() {
        WebOSPresentationTime *presentation = WebOSPlatform::instance()->presentation();
        if (presentation)
            connect(presentation, &WebOSPresentationTime::presented, this, &PresentationTime::presented);
    }

signals:
    void presented(uint32_t d2p, uint32_t p2p);

};
