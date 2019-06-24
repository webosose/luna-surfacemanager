// Copyright (c) 2014-2019 LG Electronics, Inc.
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

#ifndef WEBOSCORECOMPOSITOR_H
#define WEBOSCORECOMPOSITOR_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QObject>
#include <QList>
#include <QMap>
#include <QQuickWindow>
#include <QJSValue>

#include <qwaylandquickcompositor.h>
#include <qwaylandquicksurface.h>
#include <qwaylandoutput.h>

#include <WebOSCoreCompositor/weboskeyfilter.h>

#include "unixsignalhandler.h"
#include "webossurfaceitem.h"

class WebOSInputMethod;
class WebOSSurfaceModel;
class CompositorExtension;
class WebOSShell;
class WebOSSurfaceGroupCompositor;

class WebOSInputManager;
#ifdef MULTIINPUT_SUPPORT
class WebOSInputDevice;
#endif
class WebOSForeign;
/*!
 * \class WebOSCompositor class
 *
 * \brief This class provides a compositor implementation
 *        based on Qt Wayland.
 */

class WEBOS_COMPOSITOR_EXPORT WebOSCoreCompositor : public QObject, public QWaylandQuickCompositor
{
    Q_OBJECT

    Q_PROPERTY(WebOSSurfaceModel* surfaceModel READ surfaceModel NOTIFY surfaceModelChanged)
    // To retain backwards compatibility due to the setFullscreenSurface signature this
    // property is introduced
    Q_PROPERTY(WebOSSurfaceItem* fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)

    Q_PROPERTY(bool mouseEventEnabled READ mouseEventEnabled WRITE setMouseEventEnabled NOTIFY mouseEventEnabledChanged)

    Q_PROPERTY(WebOSInputMethod* inputMethod READ inputMethod NOTIFY inputMethodChanged)
    Q_PROPERTY(bool directRendering READ directRendering NOTIFY directRenderingChanged)
    Q_PROPERTY(bool cursorVisible READ cursorVisible NOTIFY cursorVisibleChanged) // deprecated
    Q_PROPERTY(QVariantMap surfaceItemClosePolicy READ surfaceItemClosePolicy WRITE setSurfaceItemClosePolicy NOTIFY surfaceItemClosePolicyChanged)

    Q_PROPERTY(WebOSKeyFilter* keyFilter READ keyFilter WRITE setKeyFilter NOTIFY keyFilterChanged)
    Q_PROPERTY(WebOSSurfaceItem* activeSurface READ activeSurface NOTIFY activeSurfaceChanged)

    Q_PROPERTY(QList<QObject *> foregroundItems READ foregroundItems NOTIFY foregroundItemsChanged)

public:
    enum ExtensionFlag {
        NoExtensions = 0x00,
        SurfaceGroupExtension = 0x01,
        WebOSForeignExtension = 0x02,
        // TODO: Change multinput from a config option to a flag to reduce ifdefs and
        // increase code readability
        DefaultExtensions = SurfaceGroupExtension | WebOSForeignExtension
    };
    Q_DECLARE_FLAGS(ExtensionFlags, ExtensionFlag)

    WebOSCoreCompositor(ExtensionFlags extensions = DefaultExtensions, const char *socketName = 0);
    virtual ~WebOSCoreCompositor();

    virtual void registerWindow(QQuickWindow *window, QString name = QString("primary"));

    static void logger(QtMsgType type, const QMessageLogContext &context, const QString &message);

    QWaylandQuickSurface *fullscreenSurface() const;
    WebOSSurfaceModel* surfaceModel() const;
    WebOSInputMethod* inputMethod() const;
    WebOSInputManager* inputManager() { return m_inputManager; }

    WebOSKeyFilter* keyFilter() { return m_keyFilter; }

    void surfaceAboutToBeDestroyed(QWaylandSurface *surface);

    void setAcquired(bool);
    // Override QWaylandCompositor. This will be called from Wayland::Compositor
    void directRenderingActivated(bool);
    bool directRendering() const { return m_directRendering; }

    // For debug purposes, remove when not needed
    const QList<WebOSSurfaceItem*>& getItems() const { return m_surfaces; }

    quint32 getFullscreenTick() { return ++m_fullscreenTick; }
    Q_INVOKABLE WebOSSurfaceItem* createProxyItem(const QString &appId, const QString &title, const QString &subtitle, const QString &snapshotPath);

    Q_INVOKABLE void setMouseFocus(QWaylandSurface* surface);
#ifdef MULTIINPUT_SUPPORT
    void resetMouseFocus(QWaylandSurface *surface);
#endif
    bool isMapped(WebOSSurfaceItem *item);

    bool cursorVisible() const { return m_cursorVisible; } // deprecated
    void setCursorVisible(bool visibility);
    Q_INVOKABLE void updateCursorFocus();

    void applySurfaceItemClosePolicy(QString reason, const QString &targetAppId);

    QVariantMap surfaceItemClosePolicy() { return m_surfaceItemClosePolicy; }
    void setSurfaceItemClosePolicy(QVariantMap &surfaceItemClosePolicy);

    void removeSurfaceItem(WebOSSurfaceItem* item, bool emitSurfaceDestroyed);

    //Notify which surface has pointer
    virtual void notifyPointerEnteredSurface(QWaylandSurface *surface);
    virtual void notifyPointerLeavedSurface(QWaylandSurface *surface);

    bool mouseEventEnabled() { return m_mouseEventEnabled; }
    void setMouseEventEnabled(bool enable);

#ifdef MULTIINPUT_SUPPORT
    QWaylandInputDevice *queryInputDevice(QInputEvent *inputEvent);
#endif

    /*!
     * The user can call this to register the basic qml types.
     *
     * This should be called when the compositor is not instantiated from
     * a qml plugin.
     */
    virtual void registerTypes();

    // See property comments
    WebOSSurfaceItem* fullscreen() const;
    void setFullscreen(WebOSSurfaceItem* item);
    QQuickWindow* window() const { return primaryOutput() ? static_cast<QQuickWindow *>(primaryOutput()->window()) : 0; }

    Q_INVOKABLE void emitLsmReady();

    int prepareOutputUpdate();
    void commitOutputUpdate(QQuickWindow *window, QRect geometry, int rotation, double ratio);
    void finalizeOutputUpdate();

    void initTestPluginLoader();

    QList<QWaylandInputDevice *> inputDevices() const;
    QWaylandInputDevice *inputDeviceFor(QInputEvent *inputEvent) Q_DECL_OVERRIDE;

    virtual bool getCursor(QWaylandSurface *surface, int hotSpotX, int hotSpotY, QCursor& cursor);

    WebOSKeyFilter *keyFilter() const;
    void setKeyFilter(WebOSKeyFilter *filter);
    WebOSSurfaceItem* activeSurface();

    bool setFullscreenSurface(QWaylandSurface *surface);

    Q_INVOKABLE void closeWindow(QVariant window, QJSValue payload = QJSValue());
    Q_INVOKABLE void closeWindowKeepItem(QVariant window);

    void destroyClientForWindow(QVariant window);

    QList<QObject *> foregroundItems() const;

public slots:
    void handleActiveFocusItemChanged();

signals:
    // This signal is deprecated and will be refactored away.
    void fullscreenSurfaceChanged(QWaylandSurface* oldSurface, QWaylandSurface* newSurface);
    void fullscreenSurfaceChanged();
    void fullscreenChanged();
    void homeScreenExposed();
    void inputPanelRequested();
    void inputPanelDismissed();
    void surfaceMapped(WebOSSurfaceItem* item);
    void surfaceUnmapped(WebOSSurfaceItem* item);
    void surfaceAboutToBeDestroyed(WebOSSurfaceItem* item);
    void surfaceDestroyed(WebOSSurfaceItem* item);
    void minimizeRequested(WebOSSurfaceItem* item);
    void fullscreenRequested(WebOSSurfaceItem* item);
    void surfaceModelChanged();

    void acquireChanged();
    void directRenderingChanged();

    void itemExposed(QString &item);
    void itemUnexposed(QString &item);

    void windowChanged();

    void cursorVisibleChanged(); // deprecated
    void mouseEventEnabledChanged();
    void surfaceItemClosePolicyChanged();

    //Unix signals to QT signals;
    void reloadConfig(); //SIGHUP

    void inputMethodChanged();
    void keyFilterChanged();
    void activeSurfaceChanged();

    void outputUpdateDone();

    void foregroundItemsChanged();

protected:
    virtual void surfaceCreated(QWaylandSurface *surface);

    bool acquired() { return m_acquired; }
    QHash<QString, CompositorExtension *> extensions() { return m_extensions; }

private:
    // classes
    class EventPreprocessor : public QObject
    {
    public:
        EventPreprocessor(WebOSCoreCompositor*);

    protected:
        bool eventFilter(QObject *obj, QEvent *event);

    private:
        WebOSCoreCompositor* m_compositor;
    };
    friend EventPreprocessor;

    // methods
    void checkWaylandSocket() const;

    void setCursorSurface(QWaylandSurface *surface, int hotspotX, int hotspotY, wl_client *client) Q_DECL_OVERRIDE;

    void deleteProxyFor(WebOSSurfaceItem* item);
    void initializeExtensions(WebOSCoreCompositor::ExtensionFlags extensions);

    void setInputMethod(WebOSInputMethod* inputMethod);

    bool checkSurfaceItemClosePolicy(const QString &reason, WebOSSurfaceItem *item);
    void processSurfaceItem(WebOSSurfaceItem* item, WebOSSurfaceItem::ItemState stateToBe);
    WebOSSurfaceItem* getSurfaceItemByAppId(const QString& appId);

private slots:
    /*
     * update our internal model of mapped surface in response to wayland surfaces being mapped
     * and unmapped. WindowModels use this as their source of windows.
     */
    void onSurfaceMapped();
    void onSurfaceUnmapped();
    void onSurfaceDestroyed();
    void onSurfaceSizeChanged();

    void frameSwappedSlot(); //FIXME what for

private:
    // variables
    // This is kept here for backwards compatibility.. see the deprecated signal
    QWaylandQuickSurface * m_previousFullscreenSurface;
    QWaylandQuickSurface * m_fullscreenSurface;
    WebOSSurfaceModel* m_surfaceModel;
    WebOSInputMethod* m_inputMethod;
    WebOSKeyFilter *m_keyFilter;

    /*! Holds the surfaces that have been mapped or are proxies */
    QList<WebOSSurfaceItem*> m_surfaces;
    QHash<QString, CompositorExtension *> m_extensions;
    QVariantMap m_surfaceItemClosePolicy;

    bool m_cursorVisible; // deprecated
    bool m_mouseEventEnabled;

    WebOSShell* m_shell;
    WebOSInputManager *m_inputManager;
#ifdef MULTIINPUT_SUPPORT
    WebOSInputDevice *m_inputDevicePreallocated;
    int m_lastMouseEventFrom;
#endif
    bool m_acquired;
    bool m_directRendering;

    QList<WebOSSurfaceItem*> m_surfacesOnUpdate;
    QMap<QQuickWindow *, QWaylandOutput *> m_outputs;

    //Global tick counter to get absolute time stamp for recent window model and LRU surface
    quint32 m_fullscreenTick;

    WebOSSurfaceGroupCompositor* m_surfaceGroupCompositor;

    WebOSForeign* m_foreign;

    UnixSignalHandler* m_unixSignalHandler;

    CompositorExtension *webOSWindowExtension();

    EventPreprocessor* m_eventPreprocessor;
};

#endif // WEBOSCORECOMPOSITOR_H
