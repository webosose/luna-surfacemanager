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

#ifndef COMPOSITOREXTENSIONPLUGIN_H
#define COMPOSITOREXTENSIONPLUGIN_H

#include <QObject>

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

#include <compositorextensionglobal.h>

QT_BEGIN_NAMESPACE

class CompositorExtension;

#define CompositorExtensionFactoryInterface_iid "org.openwebosproject.WebOS.Compositor.CompositorExtensionFactoryInterface.5.0"

class WEBOS_COMPOSITOR_EXT_EXPORT CompositorExtensionPlugin : public QObject
{
    Q_OBJECT
public:
    explicit CompositorExtensionPlugin(QObject *parent = 0);
    ~CompositorExtensionPlugin();

    virtual CompositorExtension *create(const QString &key, const QStringList &paramList) = 0;
};

QT_END_NAMESPACE

#endif // COMPOSITOREXTENSIONPLUGIN_H
