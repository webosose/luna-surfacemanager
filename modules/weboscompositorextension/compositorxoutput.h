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

#ifndef COMPOSITORXOUTPUT_H
#define COMPOSITORXOUTPUT_H

#include <QObject>
#include <compositorextensionglobal.h>

QT_BEGIN_NAMESPACE

class WEBOS_COMPOSITOR_EXT_EXPORT CompositorXOutput : public QObject
{
    Q_OBJECT
public:
    virtual bool stereoScopeEnabled() { return false; }
    virtual void setScreenVisible(bool visible) { Q_UNUSED(visible); }
    virtual void setStereoscopeAvailable(bool avail) { Q_UNUSED(avail); }
    virtual void clearStereoscope() { }

    virtual bool setMagnifierRun(bool run) { Q_UNUSED(run); return false; }
    virtual bool setMagnifierPos(quint32 posX, quint32 posY) { Q_UNUSED(posX); Q_UNUSED(posY); return false; }
    virtual bool setMagnification(quint32 magnification) { Q_UNUSED(magnification); return false; }
    virtual bool setMagnifierSize(const QString &size) { Q_UNUSED(size); return false; }
    virtual bool setMagnifierShape(const QString &shape) { Q_UNUSED(shape); return false; }
    virtual bool setMagnifierShapeAndSize(const QString &shape, const QString &size) { Q_UNUSED(shape); Q_UNUSED(size); return false; }

signals:
    void stereoScopeChanged();

};

QT_END_NAMESPACE

#endif // COMPOSITORXOUTPUT_H
