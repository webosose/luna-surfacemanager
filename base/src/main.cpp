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

#ifdef CURSOR_THEME
const char* EGLFS_CURSOR_DESCRIPTION = WEBOS_INSTALL_DATADIR "/icons/webos/cursors/cursor.json";
#endif

class EventFilter : public QObject {

    Q_OBJECT

public:
    EventFilter(WebOSCoreCompositor *compositor)
        : m_compositor(compositor)
        , m_useCursor(true)
        , m_cursorTimeout(0)
        , m_cursorTimer(0)
    {
        // This keeps the cursor invisible but doesn't disable mount events
        if (qEnvironmentVariableIsSet("WEBOS_CURSOR_HIDE")) {
            if (!(m_useCursor = qgetenv("WEBOS_CURSOR_HIDE").toInt() != 1))
                m_compositor->setCursorVisible(false);
        }

        if (m_useCursor) {
            // Set up the cursor timer if the timeout value is given
            if (qEnvironmentVariableIsSet("WEBOS_CURSOR_TIMEOUT")) {
                QByteArray env = qgetenv("WEBOS_CURSOR_TIMEOUT");
                m_cursorTimeout = env.toInt();
                if (m_cursorTimeout > 0) {
                    m_cursorTimer = new QTimer(this);
                    connect(m_cursorTimer, &QTimer::timeout, this, &EventFilter::onCursorTimerExpired);
                    qDebug("Cursor timeout is set as %d <- WEBOS_CURSOR_TIMEOUT=%s", m_cursorTimeout, env.constData());
                    m_cursorTimer->start(m_cursorTimeout);
                }
            }
        }
    }

    bool eventFilter(QObject *object, QEvent *event)
    {
        Q_UNUSED(object);

        // NOTE:
        // This cursor visiblity control refers to what libim has.
        // In case libim changes its behavior we have to follow it.
        if (m_useCursor) {
            switch (event->type()) {
            case QEvent::KeyPress:
                switch ((static_cast<QKeyEvent *>(event))->key()) {
                case Qt::Key_Left:
                case Qt::Key_Up:
                case Qt::Key_Right:
                case Qt::Key_Down:
                    hideCursor();
                    break;
                }
                break;

            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseMove:
            case QEvent::Wheel:
                showCursor();
                break;
            }
        }

        return false;
    }

private slots:
    void onCursorTimerExpired()
    {
        if (!m_useCursor)
            return;

        qDebug("Cursor timeout expired");
        m_compositor->setCursorVisible(false);
        hideCursor();
    }

private:
    void showCursor()
    {
        if (!m_useCursor)
            return;
        m_compositor->setCursorVisible(true);
        if (m_cursorTimer)
            m_cursorTimer->start(m_cursorTimeout);
    }

    void hideCursor()
    {
        if (!m_useCursor)
            return;
        if (m_cursorTimer)
            m_cursorTimer->stop();
        m_compositor->setCursorVisible(false);
    }

    WebOSCoreCompositor *m_compositor;
    bool m_useCursor;
    QTimer *m_cursorTimer;
    int m_cursorTimeout;
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

    QString compositorPluginName = QString::fromLocal8Bit(qgetenv("WEBOS_COMPOSITOR_PLUGIN"));
    bool usePlugin = false;
    if (!compositorPluginName.isEmpty()) {
        compositorPluginLoader = new WebOSCompositorPluginLoader(compositorPluginName);
        compositorWindow = compositorPluginLoader->compositorWindow();
        compositor = compositorPluginLoader->compositor();

        if (!compositorWindow && !compositor)
            delete compositorPluginLoader;
    }

    // Figure out the primary screen name
    int displays = qgetenv("WEBOS_COMPOSITOR_DISPLAYS").toInt();
    QString primaryScreen;
    if (displays > 1)
        primaryScreen = qgetenv("WEBOS_COMPOSITOR_PRIMARY_SCREEN");
    else
        displays = 1; // we have a single display

    if (compositorWindow) {
        qInfo() << "Using the extended compositorWindow from the plugin" << compositorPluginName;
        usePlugin = true;
    } else {
        qInfo() << "Using WebOSCompositorWindow (default compositor window)";
        compositorWindow = new WebOSCompositorWindow(primaryScreen);
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
    compositor->registerWindow(compositorWindow, "window-0");
    compositor->registerTypes();

    compositorWindow->engine()->addImportPath(QStringLiteral("qrc:/"));
    QResource::registerResource(WEBOS_INSTALL_QML "/WebOSCompositorBase/WebOSCompositorBase.rcc");
    QResource::registerResource(WEBOS_INSTALL_QML "/WebOSCompositor/WebOSCompositor.rcc");

    compositorWindow->setCompositor(compositor);
#ifdef USE_QRESOURCES
    compositorWindow->setCompositorMain(QUrl("qrc:/WebOSCompositorBase/main.qml"));
#else
    compositorWindow->setCompositorMain(QUrl("file://" WEBOS_INSTALL_QML "/WebOSCompositorBase/main.qml"));
#endif

    windowCount++;
    compositorWindow->showWindow();

    // Extra windows
    if (displays > 1) {
        qInfo() << "Initializing extra windows, expected" << displays - 1;
        QList<WebOSCompositorWindow *> extraWindows = WebOSCompositorWindow::initializeExtraWindows(primaryScreen, displays - 1, usePlugin ? compositorPluginLoader : nullptr);
        for (int i = 0; i < extraWindows.size(); i++) {
            WebOSCompositorWindow *extraWindow = extraWindows.at(i);
            QString name = QString("window-%1").arg(i + 1);
            // FIXME: Consider adding below if we need to call registerWindow
            // for an extra window
            //extraWindow->installEventFilter(new EventFilter(compositor));
            compositor->registerWindow(extraWindow, name);
            extraWindow->setCompositor(compositor);
#ifdef USE_QRESOURCES
            extraWindow->setCompositorMain(QUrl("qrc:/WebOSCompositorBase/main2.qml"));
#else
            extraWindow->setCompositorMain(QUrl("file://" WEBOS_INSTALL_QML "/WebOSCompositorBase/main2.qml"));
#endif
            windowCount++;
            extraWindow->showWindow();
            qInfo() << "Initialized an extra window" << extraWindow << "bound to wayland display" << name;
        }

        // Focus the main window
        compositorWindow->requestActivate();
    }

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
