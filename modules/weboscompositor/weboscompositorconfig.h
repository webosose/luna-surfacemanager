// Copyright (c) 2014-2020 LG Electronics, Inc.
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

#ifndef WEBOSCOMPOSITORCONFIG_H
#define WEBOSCOMPOSITORCONFIG_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QObject>
#include <QList>
#include <QHash>
#include <QString>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>

class WEBOS_COMPOSITOR_EXPORT WebOSCompositorConfig : public QObject
{
    Q_OBJECT
public:
    ~WebOSCompositorConfig();

    static WebOSCompositorConfig* instance();

    Q_INVOKABLE void dump() const;

    // Compositor plugin to use
    QString compositorPlugin() const { return m_compositorPlugin; }

    // Comma-separated list of compositor extensions to use
    QString compositorExtensions() const { return m_compositorExtensions; }

    // Name of the primary screen
    QString primaryScreen() const { return m_primaryScreen; }

    // Display configuration JSON and convenient container for JSON objects per display
    // The JSON is in the extended format of QT_QPA_EGLFS_CONFIG.
    // [
    //     {
    //         "device": "<display-device>",
    //         "outputs": [
    //             {
    //                 "name": "<display-name>",
    //                 "geometry": "<geometry-string>",
    //                 ...
    //             },
    //             ...
    //         ]
    //     },
    //     ...
    // ]
    QJsonDocument displayConfig() const { return m_displayConfig; }
    QList<QString> outputList() const { return m_outputList; }
    QHash<QString, QJsonObject> outputConfigs() const { return m_outputConfigs; }

    // Number of displays
    int displayCount() const { return m_displayCount; }

    // Default geometryString (= geometryString of the primary screen)
    QString geometryString() const { return m_geometryString; }

    // Source QML for the primary screen
    QUrl source() const { return m_source; }

    // Default source QML for extra screens if not specified
    QUrl source2() const { return m_source2; }

    // Hide cursor if set to 1
    bool cursorHide() const { return m_cursorHide; }

    // Cursor timeout in milli-seconds
    int cursorTimeout() const { return m_cursorTimeout; }

    // Exit on QML warning if set to 1
    bool exitOnQmlWarn() const { return m_exitOnQmlWarn; }

private:
    WebOSCompositorConfig();

    QString m_compositorPlugin;
    QString m_compositorExtensions;

    QString m_primaryScreen;

    QJsonDocument m_displayConfig;
    QList<QString> m_outputList;
    QHash<QString, QJsonObject> m_outputConfigs;

    int m_displayCount;
    QString m_geometryString;
    QUrl m_source;
    QUrl m_source2;

    bool m_cursorHide;
    int m_cursorTimeout;

    bool m_exitOnQmlWarn;
};

#endif
