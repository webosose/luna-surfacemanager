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

#include "weboskeyfilter.h"
#include <QDebug>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QMetaProperty>

WebOSKeyFilter::WebOSKeyFilter(QObject *parent)
    : QObject(parent)
    , m_disallowRelease(false)
    , m_wasAutoRepeat(false)
    , m_properties(0)
    , m_webos(0)
    , m_webosKeyPolicy(0)
{
}

WebOSKeyFilter::~WebOSKeyFilter()
{
    reset();
}

bool WebOSKeyFilter::handleKeyEvent(int keycode, bool pressed, bool autoRepeat)
{
    QVariant key(keycode);
    QVariant ret;

    m_wasAutoRepeat = autoRepeat;

    if (pressed) {
        if (Q_UNLIKELY(m_disallowRelease))
            m_disallowRelease = false;
    }
    else {
        if (Q_UNLIKELY(m_disallowRelease))
            return true;
    }

    // pre-process; this has to be done before handling key event
    if (Q_LIKELY(!m_preProcess.isEmpty())) {
        // m_preProcess should have one of following prototypes:
        //  - QVariant func(QVariant key, QVariant pressed, QVariant m_wasAutoRepeat);
        if (QMetaObject::invokeMethod(const_cast<WebOSKeyFilter*>(this), m_preProcess.toLatin1().constData(),
                                   Q_RETURN_ARG(QVariant, ret),
                                   Q_ARG(QVariant, key),
                                   Q_ARG(QVariant, pressed),
                                   Q_ARG(QVariant, m_wasAutoRepeat))) {

            switch (ret.toInt()) {
            case WebOSKeyPolicy::NotAccepted:
                return false;
            case WebOSKeyPolicy::Accepted:
                return true;
            case WebOSKeyPolicy::NextPolicy:
                break;
            }
        } else {
            // no appropriate method registered
            qWarning() << "Failed to invokeMethod preProcess," << m_preProcess;
        }
    }

    // main key handler
    if (Q_LIKELY(!m_handlerList.isEmpty())) {
        QList<QPair<QString, QQmlEngine*>>::iterator iter;
        for (iter = m_handlerList.begin(); iter != m_handlerList.end(); ++iter) {
            QJSValueList args;
            args << keycode << pressed << m_wasAutoRepeat;

            QString handlerName = (*iter).first;
            QQmlEngine *engine = (*iter).second;
            if (engine == NULL) {
                qWarning() << "Engine is null for" << handlerName;
                continue;
            }

            QJSValue func = engine->globalObject().property(handlerName);

            if (!func.isCallable()) {
                qWarning() << "Handler" << handlerName << "is not callable";
                continue;
            }

            QJSValue callResult = func.call(args);
            if (callResult.isError() || callResult.isUndefined()) {
                qWarning() << "Error calling" << handlerName << ":" << callResult.toString();
                continue;
            }

            switch (callResult.toInt()) {
            case WebOSKeyPolicy::NotAccepted:
                return false;
            case WebOSKeyPolicy::Accepted:
                return true;
            case WebOSKeyPolicy::NextPolicy:
                break;
            }
        }
    }

    // fallback; this is default handler for product
    if (Q_LIKELY(!m_fallback.isEmpty())) {
        // m_fallback should have one of following prototypes:
        //  - QVariant func(QVariant key, QVariant pressed, QVariant m_wasAutoRepeat);
        if (QMetaObject::invokeMethod(const_cast<WebOSKeyFilter*>(this), m_fallback.toLatin1().constData(),
                                   Q_RETURN_ARG(QVariant, ret),
                                   Q_ARG(QVariant, key),
                                   Q_ARG(QVariant, pressed),
                                   Q_ARG(QVariant, m_wasAutoRepeat))) {

            switch (ret.toInt()) {
            case WebOSKeyPolicy::Accepted:
                return true;
            case WebOSKeyPolicy::NotAccepted:
            default:
                return false;
            }
        } else {
            // no appropriate method registered
            qWarning() << "Failed to invokeMethod fallback," << m_fallback;
        }
    }

    return false;
}

void WebOSKeyFilter::keyFocusChanged()
{
    // Disallow release event unless the previous one was an auto-repeat event.
    // It will be allowed once any press event received.
    if (!m_wasAutoRepeat)
        m_disallowRelease = true;
}

bool WebOSKeyFilter::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    QEvent::Type type = event->type();
    if (type == QEvent::KeyPress || type == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        return handleKeyEvent(keyEvent->key(), (type == QEvent::KeyPress), keyEvent->isAutoRepeat());
    }
    return false;
}

void WebOSKeyFilter::reset() {
    QMapIterator<QString, QQmlEngine*> it(m_engineMap);
    while (it.hasNext()) {
        it.next();
        QQmlEngine *engine = it.value();
        delete engine;
    }
    m_handlerList.clear();
    m_engineMap.clear();
}

void WebOSKeyFilter::loadKeyFilters(QJSValue keyFilters, QObject *properties)
{
    if (!m_keyFilters.equals(keyFilters) || m_properties != properties) {
        m_keyFilters = keyFilters;
        m_properties = properties;

        reset();

        /*
         * KeyFilters Data Format (Json Array, {absolute path, function name})
         *
         * [{"/usr/lib/qt5/qml/KeyFilters/bootup.js" : "handleBootUp"},
         *  {"/usr/lib/qt5/qml/KeyFilters/screenSaver.js" : "handleScreenSaver"},
         *  {"/usr/lib/qt5/qml/KeyFilters/accessibiilty.js" : "handleAccessibiilty"}]
         */
        QJsonDocument doc = QJsonDocument::fromVariant(m_keyFilters.toVariant());

        if (!doc.isNull()) {
            if (doc.isArray()) {
                QJsonArray array = doc.array();
                foreach (const QJsonValue & jsonValue, array) {
                    QString filePath;
                    QString handlerName;

                    foreach (const QString & key, jsonValue.toObject().keys()) {
                        if (key.compare("file") == 0)
                            filePath = jsonValue.toObject().value(key).toString();
                        else if (key.compare("handler") == 0)
                            handlerName = jsonValue.toObject().value(key).toString();
                        else
                            qWarning() << "Invalid property:" << key;
                    }

                    if (filePath.isEmpty() || handlerName.isEmpty()) {
                        QJsonDocument jsonDoc(jsonValue.toObject());
                        qWarning() << "Mandatory field is/are empty. Please check the configd-data." << QString(jsonDoc.toJson());
                        continue;
                    }

                    QQmlEngine *engine;
                    if (m_engineMap.contains(filePath))
                        engine = m_engineMap.value(filePath);
                    else
                        engine = new QQmlEngine();

                    if (!loadFile(filePath, engine)) {
                        qWarning() << "loadFile() fails while processing" << filePath;
                        continue;
                    }

                    if (!setProperties(m_properties, engine)) {
                        qWarning() << "setProperties() fails while processing with engine for" << filePath;
                        continue;
                    }

                    QPair<QString, QQmlEngine*> pair;
                    pair.first = handlerName;
                    pair.second = engine;
                    m_handlerList.append(pair);

                    m_engineMap.insert(filePath, engine);
                }

                QMapIterator<QString, QQmlEngine*> iter(m_engineMap);
                while (iter.hasNext()) {
                    iter.next();
                    QQmlEngine *engine = iter.value();
                    QJSValue func = engine->globalObject().property("init");
                    if (func.isCallable()) {
                        QJSValue callResult = func.call();
                        if (callResult.isError())
                            qWarning() << "Error calling init() :" << callResult.toString();
                    }
                }
            }
            else {
                qWarning() << "KeyFilters are not a JSON array";
            }
        }
        else {
            qWarning() << "KeyFilters are invalid";
        }
    }
}

bool WebOSKeyFilter::loadFile(QString filePath, QQmlEngine *engine)
{
    QFile file(filePath);
    if (engine == NULL) {
        qWarning() << "Engine is null";
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open input file:" << filePath;
        return false;
    }

    QString sourceCode = QString::fromUtf8(file.readAll());
    QJSValue evaluation = engine->evaluate(sourceCode);

    if (evaluation.isError()) {
        qWarning() << "Uncaught exception at line"
                   << evaluation.property("lineNumber").toInt() << "in" << filePath
                   << ":" << evaluation.toString();
        return false;
    }

    return true;
}

bool WebOSKeyFilter::setProperties(QObject *properties, QQmlEngine *engine)
{
    /*
     * Properties Data Format
     *
     * readonly property QtObject properties: QtObject {
     *     readonly property bool smartTvReady : ysm.smartTvReady
     *     readonly property var homePressed: keyFilter.homePressed
     * }
     */
    if (engine == NULL) {
        qWarning() << "Engine is null";
        return false;
    }

    if (properties != NULL) {
        for (int i = 0; i < properties->metaObject()->propertyCount(); ++i) {
            QString key = properties->metaObject()->property(i).name();
            if (key == "objectName")
                continue;

            QVariant value = properties->property(key.toLatin1().data());
            QObject *obj = qvariant_cast<QObject *>(value);
            engine->globalObject().setProperty(key, engine->newQObject(obj));
        }
    }

    if (m_webos == NULL) {
        m_webos = new WebOS;
        QQmlEngine::setObjectOwnership((QObject*)m_webos, QQmlEngine::CppOwnership);
    }
    QJSValue webosJSValue = engine->newQObject(m_webos);

    const QMetaObject *metaObject = m_webos->metaObject();
    for (int ii = 0, eii = metaObject->enumeratorCount(); ii < eii; ++ii) {
        QMetaEnum enumerator = metaObject->enumerator(ii);

        for (int jj = 0, ejj = enumerator.keyCount(); jj < ejj; ++jj) {
            webosJSValue.setProperty(QString::fromUtf8(enumerator.key(jj)), enumerator.value(jj));
        }
    }
    engine->globalObject().setProperty("WebOS", webosJSValue);

    if (m_webosKeyPolicy == NULL) {
        m_webosKeyPolicy = new WebOSKeyPolicy;
        QQmlEngine::setObjectOwnership((QObject*)m_webosKeyPolicy, QQmlEngine::CppOwnership);
    }
    QJSValue keyPolicyJSValue = engine->newQObject(m_webosKeyPolicy);

    metaObject = m_webosKeyPolicy->metaObject();
    for (int ii = 0, eii = metaObject->enumeratorCount(); ii < eii; ++ii) {
        QMetaEnum enumerator = metaObject->enumerator(ii);

        for (int jj = 0, ejj = enumerator.keyCount(); jj < ejj; ++jj) {
            keyPolicyJSValue.setProperty(QString::fromUtf8(enumerator.key(jj)), enumerator.value(jj));
        }
    }
    engine->globalObject().setProperty("KeyPolicy", keyPolicyJSValue);

    return true;
}
