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

#include "webosscreenshot.h"
#include "webossurfaceitem.h"
#include "weboscompositortracer.h"

#ifdef QT_OPENGL_ES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <GL/gl.h>
#endif

#include <QImage>
#include <QGuiApplication>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QQuickWindow>

WebOSScreenShot::WebOSScreenShot()
    : m_target(Q_NULLPTR)
    , m_format("BMP")
{
}

WebOSScreenShot::ScreenShotErrors WebOSScreenShot::take()
{
    PMTRACE_FUNCTION;
    QQuickWindow* win = qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow());
    if (win == nullptr) {
        // try to take screenshot but the active window is not a quick window
        // (unlikely)
        emit screenShotError(INVALID_ACTIVE_WINDOW);
        return INVALID_ACTIVE_WINDOW;
    }

    if (!isWritablePath(m_path)) {
        emit screenShotError(INVALID_PATH);
        return INVALID_PATH;
    }

    QImage img;
    if (m_target) {
        if (!m_target->surface()) {
            emit screenShotError(NO_SURFACE);
            return NO_SURFACE;
        }
        m_size = m_target->surface()->size();

        img = QImage(m_size, QImage::Format_ARGB32_Premultiplied);

        // gets destroyed automatically after run()
        ScreenShotTask* task = new ScreenShotTask(this, &img);

        // we need to wait for the runnable to finish
        // we could make this api asynchronous instead
        win->scheduleRenderJob(task, QQuickWindow::AfterRenderingStage);
        // this will render the scene graph and trigger the runnable to run
        win->grabWindow();
    } else {
        // Grab full window. we could use the task as well,
        // but I think we should re-use Qt code if available.
        m_size = win->size();
        img = win->grabWindow();
    }

    if (img.save(m_path, m_format.toStdString().c_str())) {
        emit screenShotSaved(m_path);
        return SUCCESS;
    } else {
        emit screenShotError(UNABLE_TO_SAVE);
        return UNABLE_TO_SAVE;
    }
}

void WebOSScreenShot::unsetTarget()
{
    setTarget(nullptr);
}

void WebOSScreenShot::setTarget(WebOSSurfaceItem* target)
{
    if (target != m_target) {
        // make sure we don't hold a stale pointer when the surface item gets destroyed
        if (m_target)
            disconnect(m_target, SIGNAL(destroyed()), this, SLOT(unsetTarget()));
        if (target)
            connect(target, SIGNAL(destroyed()), this, SLOT(unsetTarget()));

        m_target = target;
        emit targetChanged();
    }
}

void WebOSScreenShot::setPath(const QString& path)
{
    QFileInfo fileInfo(path);
    if (fileInfo.absoluteFilePath() != m_path) {
        m_path = fileInfo.absoluteFilePath();
        emit pathChanged();
    }
}

void WebOSScreenShot::setFormat(const QString& format)
{
    if (format != m_format) {
        m_format = format;
        emit formatChanged();
    }
}

bool WebOSScreenShot::isWritablePath(const QString path)
{
    QFileInfo pathFileInfo(path);
    QFileInfo dirFileInfo(pathFileInfo.absoluteDir().path());

    if (!path.isEmpty() && dirFileInfo.exists() && dirFileInfo.permission(QFile::WriteUser))
        return true;
    else
        return false;
}

ScreenShotTask::ScreenShotTask(WebOSScreenShot* source, QImage* destination)
    : m_source(source), m_image(destination)
{ }

void ScreenShotTask::run() {
    PMTRACE_FUNCTION;
    if (m_source) {
        WebOSSurfaceItem *target = m_source->target();
        if (target) {
            GLuint fbo = 0;
            GLuint texture = (GLuint) target->texture();

#ifdef QT_OPENGL_ES
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glBindTexture(GL_TEXTURE_2D, texture);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

            readPixels();

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &fbo);
#else
            //TODO : Not verified yet for Desktop case.
            glBindTexture(GL_TEXTURE_2D, texture);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, m_image->bits());
            glBindTexture(GL_TEXTURE_2D, 0);
#endif
        } else {
            // Nothing to capture as the target has been unset before entering here.
        }
    }
    emit done();
}

void ScreenShotTask::readPixels()
{
    PMTRACE_FUNCTION;
    while (glGetError());
    glReadPixels(0, 0, m_image->width(), m_image->height(), GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_image->bits());
    /* GL_BGRA_EXT is not supported for glReadPixels */
    if (glGetError())
    {
        glReadPixels(0, 0, m_image->width(), m_image->height(), GL_RGBA, GL_UNSIGNED_BYTE, m_image->bits());
        *m_image = m_image->rgbSwapped();
    }
}
