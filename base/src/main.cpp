// Copyright (c) 2017-2020 LG Electronics, Inc.
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
#include <QResource>
#include <QQmlEngine>

#include <glib.h>

#include "weboscompositorpluginloader.h"
#include "weboscompositorwindow.h"
#include "weboscorecompositor.h"
#include "weboscompositorconfig.h"

#ifdef CURSOR_THEME
const char* EGLFS_CURSOR_DESCRIPTION = WEBOS_INSTALL_DATADIR "/icons/webos/cursors/cursor.json";
#endif

class EventFilter : public QObject {

    Q_OBJECT

public:
    EventFilter(WebOSCoreCompositor *compositor)
        : m_compositor(compositor)
        , m_cursorTimer(0)
    {
        m_hideCursor = WebOSCompositorConfig::instance()->cursorHide();
        // This keeps the cursor invisible but doesn't disable mount events
        if (m_hideCursor) {
            qDebug("Set cursor visibility off by config");
            m_compositor->setCursorVisible(false);
        } else {
            // Set up the cursor timer if the timeout value is given
            m_cursorTimeout = WebOSCompositorConfig::instance()->cursorTimeout();
            if (m_cursorTimeout > 0) {
                m_cursorTimer = new QTimer(this);
                connect(m_cursorTimer, &QTimer::timeout, this, &EventFilter::onCursorTimerExpired);
                qDebug("Cursor timeout is set as %d", m_cursorTimeout);
                m_cursorTimer->start(m_cursorTimeout);
            }
        }
    }

    bool eventFilter(QObject *object, QEvent *event)
    {
        Q_UNUSED(object);

        // NOTE:
        // This cursor visiblity control refers to what libim has.
        // In case libim changes its behavior we have to follow it.
        if (!m_hideCursor) {
            switch (event->type()) {
            case QEvent::KeyPress:
                switch ((static_cast<QKeyEvent *>(event))->key()) {
                case Qt::Key_Left:
                case Qt::Key_Up:
                case Qt::Key_Right:
                case Qt::Key_Down:
                    hideCursor();
                    break;
                default:
                    break;
                }
                break;

            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseMove:
            case QEvent::Wheel:
                showCursor();
                break;
            default:
                break;
            }
        }

        return false;
    }

private slots:
    void onCursorTimerExpired()
    {
        if (m_hideCursor)
            return;

        qDebug("Cursor timeout expired");
        m_compositor->setCursorVisible(false);
        hideCursor();
    }

private:
    void showCursor()
    {
        if (m_hideCursor)
            return;
        m_compositor->setCursorVisible(true);
        if (m_cursorTimer)
            m_cursorTimer->start(m_cursorTimeout);
    }

    void hideCursor()
    {
        if (m_hideCursor)
            return;
        if (m_cursorTimer)
            m_cursorTimer->stop();
        m_compositor->setCursorVisible(false);
    }

    WebOSCoreCompositor *m_compositor;
    bool m_hideCursor = false;
    QTimer *m_cursorTimer;
    int m_cursorTimeout = 0;
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

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QGuiApplication app(argc, argv);

    WebOSCompositorWindow *compositorWindow = NULL;
    WebOSCoreCompositor *compositor = NULL;
    WebOSCompositorPluginLoader *compositorPluginLoader = NULL;
    int windowCount = 0;

    QString compositorPluginName = WebOSCompositorConfig::instance()->compositorPlugin();
    bool usePlugin = false;
    if (!compositorPluginName.isEmpty()) {
        compositorPluginLoader = new WebOSCompositorPluginLoader(compositorPluginName);
        compositorWindow = compositorPluginLoader->compositorWindow(WebOSCompositorConfig::instance()->primaryScreen(), WebOSCompositorConfig::instance()->geometryString());
        compositor = compositorPluginLoader->compositor();

        if (!compositorWindow && !compositor)
            delete compositorPluginLoader;
    }

    if (compositorWindow) {
        qInfo() << "Using the extended compositorWindow from the plugin" << compositorPluginName;
        usePlugin = true;
    } else {
        qInfo() << "Using WebOSCompositorWindow (default compositor window)";
        compositorWindow = new WebOSCompositorWindow(WebOSCompositorConfig::instance()->primaryScreen());
    }

    if (compositor) {
        qInfo() << "Using the extended compositor from the plugin" << compositorPluginName;
    } else {
        qInfo() << "Using WebOSCoreCompositor (default compositor)";
        compositor = new WebOSCoreCompositor(WebOSCoreCompositor::WebOSForeignExtension);
    }

    /* https://doc.qt.io/qt-5/qobject.html#installEventFilter
       - "If multiple event filters are installed on a single object,
          the filter that was installed last is activated first."
       If there is an extended compositor, it can have event filters.
       Considering inheritance, the extended event filters should be called first,
       so that they can decide consume or pass events to base's event filter.
       For that, the base's event filter should be installed prior to 'registerWindow'
       where extended compositor installs filters. */
    compositorWindow->installEventFilter(new EventFilter(compositor));

    compositor->create();
    compositor->registerWindow(compositorWindow, WebOSCompositorConfig::instance()->primaryScreen());
    compositor->registerTypes();

    compositorWindow->engine()->addImportPath(QStringLiteral("qrc:/"));
    QResource::registerResource(WEBOS_INSTALL_QML "/WebOSCompositorBase/WebOSCompositorBase.rcc");
    QResource::registerResource(WEBOS_INSTALL_QML "/WebOSCompositor/WebOSCompositor.rcc");

    compositorWindow->setCompositor(compositor);
    compositorWindow->setCompositorMain(WebOSCompositorConfig::instance()->source());

    windowCount++;
    compositorWindow->showWindow();

    // Extra windows
    int displays = WebOSCompositorConfig::instance()->displayCount();
    if (displays > 1) {
        qInfo() << "Initializing extra windows, expected" << displays - 1;
        QList<WebOSCompositorWindow *> extraWindows = WebOSCompositorWindow::initializeExtraWindows(compositor, displays - 1, usePlugin ? compositorPluginLoader : nullptr);
        for (int i = 0; i < extraWindows.size(); i++) {
            WebOSCompositorWindow *extraWindow = extraWindows.at(i);
            windowCount++;
            extraWindow->showWindow();
            qInfo() << "Initialized an extra window" << extraWindow;
        }

        // Focus the main window
        compositorWindow->requestActivate();
    }

    // Optional post initialization after the compositor and compositor windows get initialized
    compositor->postInit();

#ifdef UPSTART_SIGNALING
    compositor->emitLsmReady();
#endif

    // Create $XDG_RUNTIME_DIR/surface-manager.windowcount
    QFile info(QString("%1/surface-manager.windowcount").arg(qgetenv("XDG_RUNTIME_DIR").constData()));
    qInfo() << "Writing window count" << windowCount << "to" << info.fileName();
    if (info.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&info);
        out << windowCount << '\n';
        info.close();
    } else {
        qWarning() << "Could not write window count:" << info.errorString();
    }

    g_timeout_add(10000, (GSourceFunc)deferredDeleter, NULL);

    return app.exec();
}

#include "main.moc"
