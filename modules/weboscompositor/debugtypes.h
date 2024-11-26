// Copyright (c) 2020-2024 LG Electronics, Inc.
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

#ifndef DEBUGTYPES_H
#define DEBUGTYPES_H

#include <QList>
#include <QObject>
#include <QPoint>
#include <qqmllist.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEventPoint>
#endif

class DebugTouchPointPrivate;

class DebugTouchPoint : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int touchId READ touchId)
    Q_PROPERTY(QPointF pos READ pos)
    Q_PROPERTY(QPointF normalizedPos READ normalizedPos)
    Q_PROPERTY(TouchPointState state READ state)

public:
    enum TouchPointState {
        TouchPointPressed,
        TouchPointMoved,
        TouchPointStationary,
        TouchPointReleased
    };
    Q_ENUM(TouchPointState);

    explicit DebugTouchPoint(QObject *parent = nullptr);
    explicit DebugTouchPoint(int id);
    DebugTouchPoint(const DebugTouchPoint &other);
#ifdef Q_COMPILER_RVALUE_REFS
    DebugTouchPoint(DebugTouchPoint &&other) Q_DECL_NOEXCEPT
        : d(nullptr)
    { qSwap(d, other.d); }
    DebugTouchPoint &operator=(DebugTouchPoint &&other) Q_DECL_NOEXCEPT
    { qSwap(d, other.d); return *this; }
#endif
    ~DebugTouchPoint() override;

    DebugTouchPoint &operator=(const DebugTouchPoint &other)
    { if ( d != other.d ) { DebugTouchPoint copy(other); swap(copy); } return *this; }

    void swap(DebugTouchPoint &other) Q_DECL_NOEXCEPT
    { qSwap(d, other.d); }

    int touchId() const;
    QPointF pos() const;
    QPointF normalizedPos() const;
    TouchPointState state() const;

    // internal
    void setPos(const QPointF &normalizedPos);
    void setNormalizedPos(const QPointF &normalizedPos);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void setState(QEventPoint::State state);
#else
    void setState(Qt::TouchPointState state);
#endif

private:
    DebugTouchPointPrivate *d;
};
Q_DECLARE_METATYPE(DebugTouchPoint)

class DebugTouchEvent : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<DebugTouchPoint> touchPoints READ touchPoints)

public:
    explicit DebugTouchEvent(QObject *parent = 0);
    DebugTouchEvent(const DebugTouchEvent& other);

    ~DebugTouchEvent() override;

    QQmlListProperty<DebugTouchPoint> touchPoints();
    QList<DebugTouchPoint *> _touchPoints() const { return m_touchPoints; }

    // internal
    void appendDebugTouchPoint(DebugTouchPoint *point);

private:
    QList<DebugTouchPoint *> m_touchPoints;
};
Q_DECLARE_METATYPE(DebugTouchEvent)

#endif // DEBUGTYPES_H
