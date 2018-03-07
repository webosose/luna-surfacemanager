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

#ifndef WEBOSSCREENSHOT_H
#define WEBOSSCREENSHOT_H

#include <QObject>
#include <QSize>
#include <QRunnable>

#include <WebOSCoreCompositor/weboscompositorexport.h>

class WebOSSurfaceItem;

class WEBOS_COMPOSITOR_EXPORT WebOSScreenShot : public QObject
{
    Q_OBJECT
    Q_FLAGS(ScreenShotErrors)
    Q_PROPERTY(WebOSSurfaceItem* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString format READ format WRITE setFormat NOTIFY formatChanged)
    Q_PROPERTY(QSize size READ size)

public:
    enum ScreenShotError {
        SUCCESS = 0,
        NO_SURFACE,
        INVALID_PATH,
        UNABLE_TO_SAVE,
        INVALID_ACTIVE_WINDOW
    };
    Q_DECLARE_FLAGS(ScreenShotErrors, ScreenShotError)

    WebOSScreenShot();

    virtual void setTarget(WebOSSurfaceItem* target);
    virtual WebOSSurfaceItem* target() const { return m_target; }

    QSize size() const { return m_size; }

    QString path() const { return m_path; }
    void setPath(const QString& path);

    QString format() const {return m_format; }
    void setFormat(const QString& format);

signals:
    void screenShotSaved(const QString& path);
    void screenShotError(ScreenShotErrors error);
    void targetChanged();
    void pathChanged();
    void formatChanged();

public slots:
    virtual ScreenShotErrors take();
private slots:
    void unsetTarget();

protected:
    WebOSSurfaceItem* m_target;
    QSize m_size;
    QString m_path;
    QString m_format;

private:
    bool isWritablePath(const QString path);

};

Q_DECLARE_OPERATORS_FOR_FLAGS(WebOSScreenShot::ScreenShotErrors)

class ScreenShotTask: public QObject, public QRunnable {
    Q_OBJECT
public:
    ScreenShotTask(WebOSScreenShot* source, QImage* destination);
    void run() Q_DECL_OVERRIDE;
    void readPixels();
signals:
    void done();
private:
    WebOSScreenShot* m_source;
    QImage* m_image;
};

#endif
