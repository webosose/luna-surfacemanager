// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#include "weboscompositorconfig.h"

#include <QJsonDocument>
#include <QStringList>
#include <QVariantMap>

WebOSCompositorConfig::WebOSCompositorConfig()
{

}

void WebOSCompositorConfig::load()
{
    // Check if the env spcifies an alternative base path
    QString base = QString::fromLocal8Bit(qgetenv("WEBOS_COMPOSITOR_SETTINGS_BASE"));
    if (base.isEmpty()) {
        base = "/etc/lsm";
    }
    // Construct the basic paths
    QStringList configFiles;
    configFiles << base + "/default.json";
    configFiles << base + "/machine.json";
    configFiles << base + "/product.json";

    // Is there an override absolute path the devel settings. The file is allowed to
    // be anywhere in the system to ensure that the partition is read/writable
    QString devel = QString::fromLocal8Bit(qgetenv("WEBOS_COMPOSITOR_SETTINGS_DEVEL"));
    if (!devel.isEmpty()) {
        configFiles << devel;
    } else {
        configFiles << "/var/preferences/com.webos.surfacemanager/devel.json";
    }

    Node root("root");
    foreach (QString path, configFiles) {
        QFile file(path);

        if (!file.open(QIODevice::ReadOnly)) {
            qWarning("Cannot open file %s, for memory reading", qPrintable(file.fileName()));
            continue;
        }

        if (!file.isReadable()) {
            qWarning("Unable to read file %s", qPrintable(file.fileName()));
            continue;
        }

        QJsonParseError err;
        QByteArray contents = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(contents, &err);
        if (err.error == QJsonParseError::NoError) {
            merge(&root, doc.object());
        } else {
            qWarning("Could not parse '%s', error '%s'", qPrintable(file.fileName()), qPrintable(err.errorString()));
        }
    }

    root.write(m_root);
}

QVariantMap WebOSCompositorConfig::config()
{
    return m_root["root"].toObject().toVariantMap();
}

void WebOSCompositorConfig::merge(Node* root, const QJsonObject& from)
{
    foreach (const QString& key, from.keys()) {
        if (from.value(key).isObject()) {
            Node* next = root->child(key);
            merge(next, from.value(key).toObject());
        } else {
            Node* n = root->child(key);
            n->setValue(from.value(key).toVariant());
        }
    }
}
