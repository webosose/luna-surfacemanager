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

#include <QGuiApplication>
#include <QScreen>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

#include "weboscompositorconfig.h"

static WebOSCompositorConfig* s_instance = nullptr;

WebOSCompositorConfig::WebOSCompositorConfig()
{
    m_compositorPlugin = QString::fromLatin1(qgetenv("WEBOS_COMPOSITOR_PLUGIN"));
    m_compositorExtensions = QString::fromLatin1(qgetenv("WEBOS_COMPOSITOR_EXTENSIONS"));

    m_primaryScreen = QString::fromLatin1(qgetenv("WEBOS_COMPOSITOR_PRIMARY_SCREEN"));
    if (Q_UNLIKELY(m_primaryScreen.isEmpty()))
        m_primaryScreen = QGuiApplication::primaryScreen()->name();

    if (qEnvironmentVariableIsEmpty("WEBOS_COMPOSITOR_DISPLAY_CONFIG"))
        m_displayConfig = QJsonDocument(); // empty JSON
    else
        m_displayConfig = QJsonDocument::fromJson(qgetenv("WEBOS_COMPOSITOR_DISPLAY_CONFIG"));
    for (int i = 0; i < m_displayConfig.array().size(); i++) {
        QJsonObject obj = m_displayConfig.array().at(i).toObject();
        QJsonArray outputs = obj.value(QStringLiteral("outputs")).toArray();
        for (int j = 0; j < outputs.size(); j++) {
            QJsonObject objj = outputs.at(j).toObject();
            QString name = objj.value(QStringLiteral("name")).toString();
            if (name.isEmpty())
                continue; // skipped
            else
                m_outputConfigs.insert(name, objj);
        }
    }

    // Following env variables are used if it is unable to
    // get the value from WEBOS_COMPOSITOR_DISPLAY_CONFIG.
    // * WEBOS_COMPOSITOR_DISPLAYS: Number of displays
    // * WEBOS_COMPOSITOR_GEOMETRY: Default geometryString
    // * WEBOS_COMPOSITOR_MAIN: Source QML for the primary screen
    m_displayCount = m_outputConfigs.size();
    if (Q_UNLIKELY(m_displayCount <= 0)) {
        m_displayCount = qgetenv("WEBOS_COMPOSITOR_DISPLAYS").toInt();
        if (Q_UNLIKELY(m_displayCount <= 0))
            m_displayCount = 1;
    }
    QJsonObject primary = m_outputConfigs.value(m_primaryScreen);
    if (!primary.isEmpty()) {
        m_geometryString = primary.value(QStringLiteral("geometry")).toString();
        m_source = primary.value(QStringLiteral("source")).toString();
    }
    if (Q_UNLIKELY(m_geometryString.isEmpty())) {
        m_geometryString = QString::fromLatin1(qgetenv("WEBOS_COMPOSITOR_GEOMETRY"));
        if (Q_UNLIKELY(m_geometryString.isEmpty()))
            m_geometryString = QString::fromLatin1("1920x1080+0+0r0s1");
    }
    if (Q_UNLIKELY(m_source.isEmpty())) {
        m_source = QString::fromLatin1(qgetenv("WEBOS_COMPOSITOR_MAIN"));
        if (Q_UNLIKELY(m_source.isEmpty())) {
#ifdef USE_QRESOURCES
            m_source = QUrl("qrc:/WebOSCompositorBase/main.qml");
#else
            m_source = QUrl("file://" WEBOS_INSTALL_QML "/WebOSCompositorBase/main.qml");
#endif
        }
    }
#ifdef USE_QRESOURCES
    m_source2 = QUrl("qrc:/WebOSCompositorBase/main2.qml");
#else
    m_source2 = QUrl("file://" WEBOS_INSTALL_QML "/WebOSCompositorBase/main2.qml");
#endif

    m_cursorHide = (qgetenv("WEBOS_CURSOR_HIDE").toInt() == 1);
    m_cursorTimeout = qgetenv("WEBOS_CURSOR_TIMEOUT").toInt();

    m_exitOnQmlWarn = (qgetenv("WEBOS_COMPOSITOR_EXIT_ON_QMLWARN").toInt() == 1);
}

WebOSCompositorConfig::~WebOSCompositorConfig()
{
}

WebOSCompositorConfig* WebOSCompositorConfig::instance()
{
    if (Q_UNLIKELY(!s_instance))
        s_instance = new WebOSCompositorConfig();

    return s_instance;
}

void WebOSCompositorConfig::dump() const
{
    qInfo() << "=== WebOSCompositorConfig BEGIN ===";
    qInfo() << "compositorPlugin:" << m_compositorPlugin;
    qInfo() << "compositorExtensions:" << m_compositorExtensions;
    qInfo() << "primaryScreen:" << m_primaryScreen;
    qInfo() << "displayConfig:" << m_displayConfig;
    for (QMap<QString, QJsonObject>::const_iterator i = m_outputConfigs.constBegin(); i != m_outputConfigs.constEnd(); i++)
        qInfo() << "outputConfigs:" << i.key() << i.value();
    qInfo() << "displayCount:" << m_displayCount;
    qInfo() << "geometryString:" << m_geometryString;
    qInfo() << "source:" << m_source;
    qInfo() << "source2:" << m_source2;
    qInfo() << "cursorHide:" << m_cursorHide;
    qInfo() << "cursorTimeout:" << m_cursorTimeout;
    qInfo() << "exitOnQmlWarn:" << m_exitOnQmlWarn;
    qInfo() << "=== WebOSCompositorConfig END   ===";
}
