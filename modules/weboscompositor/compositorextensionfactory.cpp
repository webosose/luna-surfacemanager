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

#include "compositorextensionfactory.h"
#include "weboscorecompositor.h"
#include "weboscompositorconfig.h"

#include <compositorextensionplugin.h>
#include <compositorextension.h>

#include <QDebug>
#include <QJsonObject>
#include <QObject>
#include <QPluginLoader>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

//We can assume that there are already plugin path to be set.
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (CompositorExtensionFactoryInterface_iid, QLatin1String("/compositorextensions"), Qt::CaseInsensitive))

QFileSystemWatcher CompositorExtensionFactory::m_fileWatcher;
WebOSCoreCompositor* CompositorExtensionFactory::m_webosCompositor;
const char* CompositorExtensionFactory::pluginDir = "/tmp/lsm_test_plugin";

QHash<QString, CompositorExtension *> CompositorExtensionFactory::create(WebOSCoreCompositor* compositor)
{
    QHash<QString, CompositorExtension *> hash;

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QStringList extensions = WebOSCompositorConfig::instance()->compositorExtensions().split(QLatin1String(","), Qt::SkipEmptyParts);
#else
    QStringList extensions = WebOSCompositorConfig::instance()->compositorExtensions().split(QLatin1String(","), QString::SkipEmptyParts);
#endif

    m_webosCompositor = compositor;

    qDebug() << "Loading the following plugins" << extensions;
    for (int i = 0; i < extensions.size(); i++) {
        CompositorExtension *extension = qLoadPlugin1<CompositorExtension, CompositorExtensionPlugin>(loader(), extensions[i], QStringList());
        if (!extension)
            continue;

        initializeExtension(extension);
        hash[extensions[i]] = extension;
    }

    return hash;
}

void CompositorExtensionFactory::watchTestPluginDir()
{
    QDir testPluginDir;
    testPluginDir.mkpath(pluginDir);

    //Watch that test-plugin is created. And then load it.
    if (m_fileWatcher.addPath(pluginDir)) {
        qDebug() << "Watch test plugin in" << testPluginDir.absolutePath();
        QObject::connect(&CompositorExtensionFactory::m_fileWatcher,
                         &QFileSystemWatcher::directoryChanged,
                         handleDirectoryChanged);
    } else {
        qWarning() << "Failed to watch " << testPluginDir.absolutePath();
    }
}

void CompositorExtensionFactory::initializeExtension(CompositorExtension*
                                                                     extension)
{
    QObject::connect(CompositorExtensionFactory::m_webosCompositor,
                     SIGNAL(homeScreenExposed()),
                     extension, SIGNAL(homeScreenExposed()));
    QObject::connect(CompositorExtensionFactory::m_webosCompositor,
                     SIGNAL(fullscreenSurfaceChanged(QWaylandSurface *,
                                                     QWaylandSurface *)),
                     extension,
                     SIGNAL(fullscreenSurfaceChanged(QWaylandSurface *,
                                                     QWaylandSurface *)));
    QObject::connect(CompositorExtensionFactory::m_webosCompositor,
                     SIGNAL(itemExposed(QString &)),
                     extension, SLOT(handleItemExposed(QString &)));
    QObject::connect(CompositorExtensionFactory::m_webosCompositor,
                     SIGNAL(itemUnexposed(QString &)),
                     extension, SLOT(handleItemUnexposed(QString &)));

    extension->setCompositor(CompositorExtensionFactory::m_webosCompositor);
}

void CompositorExtensionFactory::handleDirectoryChanged(const QString& path)
{
    Q_UNUSED(path);

    QDir::setCurrent(pluginDir);
    if (!(QFile::exists(".done"))) //File copy is still in progress ...
        return;

    QDir testPluginDir(pluginDir);
    testPluginDir.setNameFilters(QStringList()<<"*.so");

    foreach (QString fileName, testPluginDir.entryList(QDir::Files)) {
        QPluginLoader pluginLoader(testPluginDir.absoluteFilePath(fileName));
        const QJsonObject metaDataObject = pluginLoader.metaData();
        if ( metaDataObject.value(QStringLiteral("IID")) !=
             QJsonValue(QLatin1String(CompositorExtensionFactoryInterface_iid))) {

            qWarning() << "Wrong IID: " << testPluginDir.absoluteFilePath(fileName)
                     << metaDataObject.value(QStringLiteral("IID"));
            continue;
        }

        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            CompositorExtensionPlugin *testExtensionFactory =
                             qobject_cast<CompositorExtensionPlugin* >(plugin);

            CompositorExtension *testExtension =
                           testExtensionFactory->create("webos-test-extension",
                                                        QStringList());

            if (testExtension) {
                qDebug() << "Test plugin loaded. File:"
                         << testPluginDir.absoluteFilePath(fileName);

                initializeExtension(testExtension);
            }
        } else {
            qWarning() << "Test plugin loading failed.: " << pluginLoader.errorString();
        }
    }
    QObject::disconnect(&CompositorExtensionFactory::m_fileWatcher, 0, 0, 0);
}
QT_END_NAMESPACE
