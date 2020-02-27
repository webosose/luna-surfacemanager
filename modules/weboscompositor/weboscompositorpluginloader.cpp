// Copyright (c) 2018-2020 LG Electronics, Inc.
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

#include "weboscompositorpluginloader.h"

#include <QDebug>
#include <QDir>
#include <QJsonArray>

WebOSCompositorPluginLoader::WebOSCompositorPluginLoader(const QString &pluginName)
    : m_pluginName(pluginName)
    , m_compositor(nullptr)
    , m_compositorPlugin(nullptr)
{
    m_pluginLoader = load(m_pluginName);
}

WebOSCompositorPluginLoader::~WebOSCompositorPluginLoader()
{
    unload(m_pluginLoader);
    delete m_pluginLoader;
}

QPluginLoader *WebOSCompositorPluginLoader::load(const QString &pluginName)
{
    // Rules for compositor plugin
    //  - Directory path suffix: compositor
    //  - Filename: lib<pluginname>.so
    //  - IID and Keys in the plugin metadata should match.

    QDir pluginDir(WEBOS_INSTALL_QTPLUGINSDIR "/compositor");
    if (!pluginDir.exists()) {
        qWarning() << "WebOSCompositorPluginLoader: Compositor plugins directory does not exist. Plugins directory path:" << pluginDir.path();
        return NULL;
    }

    QString pluginFile = pluginDir.absoluteFilePath("lib" + pluginName + ".so");
    if (!QFile::exists(pluginFile)) {
        qWarning() << "WebOSCompositorPluginLoader: Cannot find plugin file named as" << pluginName;
        return NULL;
    }

    QPluginLoader *pluginLoader = new QPluginLoader(pluginFile);

    QString pluginKeys = pluginLoader->metaData().value("MetaData").toObject().value("Keys").toArray().first().toString();
    if (QString::compare(pluginName, pluginKeys, Qt::CaseSensitive) != 0) {
        qWarning() << "WebOSCompositorPluginLoader: Key does not match. Plugin Keys:" << pluginKeys;
        delete pluginLoader;
        return NULL;
    }

    QString pluginIid = pluginLoader->metaData().value("IID").toString();
    if (QString::compare(WebOSCompositorInterface_iid, pluginIid, Qt::CaseSensitive) != 0) {
        qWarning() << "WebOSCompositorPluginLoader: IID does not match. Plugin IID:" << pluginIid;
        delete pluginLoader;
        return NULL;
    }

    QObject *plugin = pluginLoader->instance();
    if (plugin) {
        WebOSCompositorInterface *compositorPlugin = qobject_cast<WebOSCompositorInterface *>(plugin);

        if (compositorPlugin) {
            qInfo() << "WebOSCompositorPluginLoader: Succeeded to load the compositor plugin" << pluginFile;

            if (QMetaObject::invokeMethod(compositorPlugin, "init", Qt::DirectConnection)) {
                QMetaObject::invokeMethod(compositorPlugin, "compositorExtended", Qt::DirectConnection, Q_RETURN_ARG(WebOSCoreCompositor *, m_compositor));
                if (m_compositor && !m_compositor->inherits(WebOSCoreCompositor::staticMetaObject.className())) {
                    qWarning() << "WebOSCompositorPluginLoader: Not a valid WebOSCoreCompositor instance.";
                    delete m_compositor;
                    m_compositor = nullptr;
                }

                if (m_compositor) {
                    m_compositorPlugin = compositorPlugin;
                    return pluginLoader;
                } else {
                    qWarning() << "WebOSCompositorPluginLoader: Cannot get valid WebOSCoreCompositor instance.";
                }
            } else {
                qWarning() << "WebOSCompositorPluginLoader: Cannot find init function in the compositor plugin.";
            }
        } else {
            qWarning() << "WebOSCompositorPluginLoader: Not a valid compositor plugin instance" << pluginFile;
        }
    } else {
        qWarning() << "WebOSCompositorPluginLoader: Failed to load the compositor plugin" << pluginFile << ".:" << pluginLoader->errorString();
    }

    unload(pluginLoader);
    delete pluginLoader;

    return NULL;
}

bool WebOSCompositorPluginLoader::unload(QPluginLoader *pluginLoader)
{
    if (pluginLoader && pluginLoader->isLoaded()) {
        if (pluginLoader->unload()) {
            qInfo() << "WebOSCompositorPluginLoader: Unload the plugin" << pluginLoader->fileName();
            return true;
        } else {
            qWarning() << "WebOSCompositorPluginLoader: Failed to unload the plugin" << pluginLoader->fileName();
        }
    }

    return false;
}

WebOSCoreCompositor *WebOSCompositorPluginLoader::compositor()
{
    return m_compositor;
}

WebOSCompositorWindow *WebOSCompositorPluginLoader::compositorWindow(QString screenName, QString geometryString, QSurfaceFormat *surfaceFormat)
{
    WebOSCompositorWindow *compositorWindow = nullptr;

    // Try new version first
    QMetaObject::invokeMethod(m_compositorPlugin, "compositorWindowExtended", Qt::DirectConnection,
            Q_RETURN_ARG(WebOSCompositorWindow *, compositorWindow),
            Q_ARG(QString, screenName),
            Q_ARG(QString, geometryString),
            Q_ARG(QSurfaceFormat *, surfaceFormat));
    if (compositorWindow && !compositorWindow->inherits(WebOSCompositorWindow::staticMetaObject.className())) {
        // Fallback to old version
        compositorWindow = nullptr;
        QMetaObject::invokeMethod(m_compositorPlugin, "compositorWindowExtended", Qt::DirectConnection, Q_RETURN_ARG(WebOSCompositorWindow *, compositorWindow));
        if (compositorWindow && !compositorWindow->inherits(WebOSCompositorWindow::staticMetaObject.className())) {
            qWarning() << "WebOSCompositorPluginLoader: Not a valid WebOSCompositorWindow instance.";
            delete compositorWindow;
            compositorWindow = nullptr;
        }
    }

    return compositorWindow;
}
