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

WebOSKeyFilter::WebOSKeyFilter(QObject *parent)
    : QObject(parent)
    , m_disallowRelease(false)
    , m_wasAutoRepeat(false)
{
}

WebOSKeyFilter::~WebOSKeyFilter()
{
    resetKeyFilters();
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
        QJSValueList args;
        args << keycode << pressed << m_wasAutoRepeat;
        for (int i = 0; i < m_handlerList.size(); i++) {
            QString handlerName = m_handlerList[i].first;
            QJSValue keyFilter = m_handlerList[i].second;
            if (keyFilter.isCallable()) {
                QJSValue callResult = keyFilter.call(args);
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
            } else {
                qWarning() << "Handler" << handlerName << "is not callable";
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

void WebOSKeyFilter::resetKeyFilters()
{
    m_handlerList.clear();
    qDebug() << "All keyFilters are removed";
}

void WebOSKeyFilter::addKeyFilter(QJSValue keyFilter, QString handlerName)
{
    QPair<QString, QJSValue> pair;
    pair.first = handlerName;
    pair.second = keyFilter;
    m_handlerList.append(pair);
    qDebug() << "KeyFilter added:" << handlerName;
}
