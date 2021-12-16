// Copyright (c) 2019-2021 LG Electronics, Inc.
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

#ifndef WEBOSEVENT_H
#define WEBOSEVENT_H


#include <QEvent>
#include <QWindow>
#include <QtGui/private/qguiapplication_p.h>

class WebOSMouseEvent: public QMouseEvent
{
public:
    WebOSMouseEvent(Type type, const QPointF &localPos, Qt::MouseButton button,
            Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, QWindow *window)
        : QMouseEvent(type, localPos, button, buttons, modifiers)
    {
        m_window = window;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        m_source = Qt::MouseEventSource::MouseEventSynthesizedByApplication;
#else
        QGuiApplicationPrivate::setMouseEventSource(this, Qt::MouseEventSource::MouseEventSynthesizedByApplication);
#endif
    }

    QWindow *window() const { return m_window; }

private:
    QWindow *m_window;

};

class WebOSWheelEvent: public QWheelEvent
{
public:
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    WebOSWheelEvent(const QPointF &pos, const QPointF& globalPos, QPoint pixelDelta, QPoint angleDelta,
            Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Qt::ScrollPhase phase, QWindow *window,
            bool inverted)
        : QWheelEvent(pos, globalPos, pixelDelta, angleDelta,
            buttons, modifiers, phase, inverted, Qt::MouseEventSynthesizedByApplication)
#else
    WebOSWheelEvent(const QPointF &pos, const QPointF& globalPos, QPoint pixelDelta, QPoint angleDelta,
            int qt4Delta, Qt::Orientation qt4Orientation,
            Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Qt::ScrollPhase phase, QWindow *window)
        : QWheelEvent(pos, globalPos, pixelDelta, angleDelta, qt4Delta, qt4Orientation,
            buttons, modifiers, phase, Qt::MouseEventSynthesizedByApplication)
#endif
    {
        m_window = window;
    }

    QWindow *window() const { return m_window; }

private:
    QWindow *m_window;

};

class WebOSHoverEvent: public QHoverEvent
{
public:
    WebOSHoverEvent(Type type, const QPointF &pos, const QPointF &oldPos,
            Qt::KeyboardModifiers modifiers, QWindow *window)
        : QHoverEvent(type, pos, oldPos, modifiers)
    {
        m_window = window;
    }

    QWindow *window() const { return m_window; }

private:
    QWindow *m_window = nullptr;
};

#endif
