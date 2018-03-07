// Copyright (c) 2017-2018 LG Electronics, Inc.
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
#include <QQuickWindow>
#include <QObject>
#include <QFile>
#include <QDebug>

#include <glib.h>

#include "weboscompositorpluginloader.h"
#include "weboscompositorwindow.h"
#include "weboscorecompositor.h"

#ifdef CURSOR_THEME
const char* EGLFS_CURSOR_DESCRIPTION = WEBOS_INSTALL_DATADIR "/icons/webos/cursors/cursor.json";
#endif

class EventFilter : public QObject
{
public:
    EventFilter(WebOSCoreCompositor *compositor)
        : m_compositor(compositor)
    {
        m_compositor->setCursorVisible(true);
    }

    bool eventFilter(QObject *object, QEvent *event)
    {
        Q_UNUSED(object);

        // TODO:
        // Check if this conflicts the cursor visibility control logic in IM.
        // If not, consider moving it to the event filter in WebOSCoreCompositor.
        switch (event->type()) {
        case QEvent::KeyPress:
            switch ((static_cast<QKeyEvent *>(event))->key()) {
            case Qt::Key_Left:
            case Qt::Key_Up:
            case Qt::Key_Right:
            case Qt::Key_Down:
                m_compositor->setCursorVisible(false);
                break;
            }
            break;

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::Wheel:
            m_compositor->setCursorVisible(true);
            break;
        }

        return false;
    }

private:
    WebOSCoreCompositor *m_compositor;
};

static gboolean deferredDeleter(gpointer data)
{
    Q_UNUSED(data)

    /* Process any "deleteLater" objects.
       QtDeclarative defers to delete some objects including textures from image.
       Those objects only can be removed when control returns to the event loop.
       Otherwise we have to call sendPostedEvents(QEvent::DeferredDelete) explicitly.
       In threaded renderer of QtQuick, there is already some preparation for this.
       But we are not ready to use the renderer yet.
       Please refer QObject::"deleteLater" for details. */
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);

    return true;
}

int main(int argc, char *argv[])
{
#ifdef CURSOR_THEME
    qputenv("QT_QPA_EGLFS_CURSOR", EGLFS_CURSOR_DESCRIPTION);
#endif
    qInstallMessageHandler(WebOSCoreCompositor::logger);

    // We want the main compositor window to be able to be transparent
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);

    WebOSCompositorWindow *compositorWindow = NULL;
    WebOSCoreCompositor *compositor = NULL;
    WebOSCompositorPluginLoader *compositorPluginLoader = NULL;

    QString compositorPluginName = QString::fromLocal8Bit(qgetenv("WEBOS_COMPOSITOR_PLUGIN"));
    if (!compositorPluginName.isEmpty()) {
        compositorPluginLoader = new WebOSCompositorPluginLoader(compositorPluginName);
        compositorWindow = compositorPluginLoader->compositorWindow();
        compositor = compositorPluginLoader->compositor();

        if (!compositorWindow && !compositor)
            delete compositorPluginLoader;
    }

    if (compositorWindow) {
        qInfo() << "Using the extended compositorWindow from the plugin" << compositorPluginName;
    } else {
        qInfo() << "Using WebOSCompositorWindow (default compositor window)";
        compositorWindow = new WebOSCompositorWindow();
    }

    if (compositor) {
        qInfo() << "Using the extended compositor from the plugin" << compositorPluginName;
    } else {
        qInfo() << "Using WebOSCoreCompositor (default compositor)";
        compositor = new WebOSCoreCompositor(compositorWindow, WebOSCoreCompositor::NoExtensions);
    }

    compositor->registerTypes();

    compositorWindow->setCompositor(compositor);
    compositorWindow->setCompositorMain(QUrl("file://" WEBOS_INSTALL_QML "/WebOSCompositorBase/main.qml"));

#ifdef UPSTART_SIGNALING
    compositor->emitLsmReady();
#endif

    compositorWindow->installEventFilter(new EventFilter(compositor));

    compositorWindow->showWindow();

    g_timeout_add(10000, (GSourceFunc)deferredDeleter, NULL);

    return app.exec();
}
