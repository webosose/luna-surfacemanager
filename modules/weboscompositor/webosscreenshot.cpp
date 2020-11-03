// Copyright (c) 2014-2021 LG Electronics, Inc.
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
#include "weboscompositorwindow.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <QImage>
#include <QGuiApplication>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QQuickWindow>

WebOSScreenShot::WebOSScreenShot()
    : m_target(nullptr)
    , m_format("BMP")
    , m_window(nullptr)
{
}

WebOSScreenShot::ScreenShotErrors WebOSScreenShot::take()
{
    PMTRACE_FUNCTION;
    QQuickWindow* win = m_window ? m_window : qobject_cast<QQuickWindow*>(QGuiApplication::focusWindow());
    if (win == nullptr) {
        // try to take screenshot but the active window is not a quick window
        // (unlikely)
        emit screenShotError(INVALID_ACTIVE_WINDOW);
        return INVALID_ACTIVE_WINDOW;
    }

    if (!isValidPath(m_path)) {
        emit screenShotError(INVALID_PATH);
        return INVALID_PATH;
    }

    QImage img;
    if (m_target) {
        if (!m_target->surface() || !m_target->window()) {
            emit screenShotError(NO_SURFACE);
            return NO_SURFACE;
        }
        if (m_target->hasSecuredContent()) {
            emit screenShotError(HAS_SECURED_CONTENT);
            return HAS_SECURED_CONTENT;
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
        WebOSCompositorWindow *compositorWindow = qobject_cast<WebOSCompositorWindow *>(win);
        if (compositorWindow && compositorWindow->hasSecuredContent()) {
            emit screenShotError(HAS_SECURED_CONTENT);
            return HAS_SECURED_CONTENT;
        }
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
    if (path != m_path) {
        m_path = path;
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

void WebOSScreenShot::setWindow(QQuickWindow* window)
{
    if (m_window != window) {
        m_window = window;
        emit windowChanged();
    }
}

bool WebOSScreenShot::isValidPath(const QString& path) const
{
    if (path.isEmpty())
        return false;

    QFileInfo pathFileInfo(path);

    if (!pathFileInfo.isAbsolute())
        return false;

    QFileInfo dirFileInfo(pathFileInfo.absoluteDir().path());

    if (!dirFileInfo.exists() || !dirFileInfo.permission(QFile::WriteUser))
        return false;

    return true;
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

            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glBindTexture(GL_TEXTURE_2D, texture);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

            readPixels();

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &fbo);
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
