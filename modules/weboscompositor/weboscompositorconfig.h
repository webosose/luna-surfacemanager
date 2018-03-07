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

#ifndef WEBOSCOMPOSITORCONFIG_H
#define WEBOSCOMPOSITORCONFIG_H

#include <QFile>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QVariant>

// A temporaty class to allow the combination of multiple JSON objects
// into one so that there are no duplicates. Kinda wonky but there are
// no stock solutions and this is still pretty straight forward.
class Node {

public:
    Node(const QString& key = "")
        : m_key(key)
    {
    }

    ~Node()
    {
        // Calls delete on the inserted value
        qDeleteAll(m_children);
    }


    Node* child(const QString& key)
    {
        if (!m_children.contains(key)) {
            Node* n = new Node(key);
            m_children.insert(key, n);
        }
        return m_children.value(key);
    }

    void setValue(QVariant v)
    {
        m_value = v;
    }

    void write(QJsonObject& to)
    {
        if (m_value.isValid()) {
            to[m_key] = QJsonValue::fromVariant(m_value);
        } else {
            QJsonObject j;
            foreach (Node* n, m_children) {
                n->write(j);
            }
            to[m_key] = j;
        }
    }

private:
    QString m_key;
    QVariant m_value;
    QMap<QString, Node*> m_children;
};

class WebOSCompositorConfig {

public:
    WebOSCompositorConfig();

    void load();

    QVariantMap config();

private:
    void merge(Node* root, const QJsonObject& from);
    QJsonObject m_root;
};

#endif
