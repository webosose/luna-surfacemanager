// Copyright (c) 2014-2022 LG Electronics, Inc.
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
#include <QtWaylandCompositor/QWaylandWlShellSurface>

#include "unixsignalhandler.h"
#include "webossurfaceitem.h"
#include "weboscompositorconfig.h"

class WaylandInputMethod;
class WaylandInputMethodManager;
class WebOSInputMethod;
class WebOSSurfaceModel;
class CompositorExtension;
class WebOSShell;
class WebOSSurfaceGroupCompositor;
class WebOSCompositorWindow;
class WebOSInputManager;
#ifdef MULTIINPUT_SUPPORT
class WebOSInputDevice;
#endif
class WebOSForeign;
class WebOSTablet;

/*!
 * \class WebOSCoreCompositor class
 *
 * \brief This class provides a compositor implementation
 *        based on Qt Wayland.
 */

class WebOSCoreCompositorPrivate;

class WEBOS_COMPOSITOR_EXPORT WebOSCoreCompositor : public QWaylandCompositor
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WebOSCoreCompositor)

    Q_PROPERTY(WebOSSurfaceModel* surfaceModel READ surfaceModel NOTIFY surfaceModelChanged)
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    Q_MOC_INCLUDE("webossurfacemodel.h")
#endif
    // To retain backwards compatibility due to the setFullscreenSurface signature this
    // property is introduced
    Q_PROPERTY(WebOSSurfaceItem* fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)

    Q_PROPERTY(bool mouseEventEnabled READ mouseEventEnabled WRITE setMouseEventEnabled NOTIFY mouseEventEnabledChanged)

    Q_PROPERTY(WebOSInputMethod* inputMethod READ inputMethod NOTIFY inputMethodChanged)
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    Q_MOC_INCLUDE("webosinputmethod.h")
#endif
    Q_PROPERTY(bool directRendering READ directRendering NOTIFY directRenderingChanged)
    Q_PROPERTY(bool cursorVisible READ cursorVisible NOTIFY cursorVisibleChanged) // deprecated
    Q_PROPERTY(QVariantMap surfaceItemClosePolicy READ surfaceItemClosePolicy WRITE setSurfaceItemClosePolicy NOTIFY surfaceItemClosePolicyChanged)

    Q_PROPERTY(WebOSKeyFilter* keyFilter READ keyFilter WRITE setKeyFilter NOTIFY keyFilterChanged)
    Q_PROPERTY(WebOSSurfaceItem* activeSurface READ activeSurface NOTIFY activeSurfaceChanged)

    Q_PROPERTY(QList<QObject *> windows READ windows NOTIFY windowsChanged)

    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadCompleted)
    Q_PROPERTY(bool respawned READ respawned NOTIFY respawnedChanged)

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

    void create() override;
    virtual void registerWindow(QQuickWindow *window, QString name = QString());
    void insertToWindows(WebOSCompositorWindow *);

    static void logger(QtMsgType type, const QMessageLogContext &context, const QString &message);

    WebOSSurfaceItem *fullscreenSurfaceItem() const;
    QWaylandQuickSurface *fullscreenSurface() const;
    WebOSSurfaceModel* surfaceModel() const;
    WebOSInputMethod* inputMethod() const;
    WebOSInputManager* inputManager() { return m_inputManager; }

    QSharedPointer<WebOSTablet> tabletDevice() { return m_webosTablet; }

    WebOSKeyFilter* keyFilter() { return m_keyFilter; }

    void setAcquired(bool);
    // Override QWaylandCompositor. This will be called from Wayland::Compositor
    void directRenderingActivated(bool);
    bool directRendering() const { return m_directRendering; }

    const QList<WebOSSurfaceItem *> getItems(WebOSCompositorWindow *window = nullptr) const;

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

    void addSurfaceItem(WebOSSurfaceItem *item);
    void removeSurfaceItem(WebOSSurfaceItem* item, bool emitSurfaceDestroyed);
    void handleSurfaceUnmapped(WebOSSurfaceItem* item);

    //Notify which surface has pointer
    virtual void notifyPointerEnteredSurface(QWaylandSurface *surface);
    virtual void notifyPointerLeavedSurface(QWaylandSurface *surface);

    bool mouseEventEnabled() { return m_mouseEventEnabled; }
    void setMouseEventEnabled(bool enable);

#ifdef MULTIINPUT_SUPPORT
    QWaylandSeat *queryInputDevice(QInputEvent *inputEvent);
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
    QQuickWindow* window() const { return defaultOutput() ? static_cast<QQuickWindow *>(defaultOutput()->window()) : 0; }

    Q_INVOKABLE void emitLsmReady();

    int prepareOutputUpdate();
    void commitOutputUpdate(QQuickWindow *window, QRect geometry, int rotation, double ratio);
    void finalizeOutputUpdate();

    void initTestPluginLoader();

    QList<QWaylandSeat *> inputDevices() const;
    QWaylandSeat *keyboardDeviceForWindow(QQuickWindow *window);
    QWaylandSeat *keyboardDeviceForDisplayId(int displayId);

    virtual bool getCursor(QWaylandSurface *surface, int hotSpotX, int hotSpotY, QCursor& cursor);

    WebOSKeyFilter *keyFilter() const;
    void setKeyFilter(WebOSKeyFilter *filter);
    WebOSSurfaceItem* activeSurface();

    Q_INVOKABLE void closeWindow(QVariant window, QJSValue payload = QJSValue());
    Q_INVOKABLE void closeWindowKeepItem(QVariant window);

    void destroyClientForWindow(QVariant window);

    QList<QObject *> windows() const;
    WebOSCompositorWindow *window(int displayId);
    Q_INVOKABLE QList<QObject *> windowsInCluster(QString clusterName);
    void updateWindowPositionInCluster();

    bool autoStart() const { return m_autoStart; }
    void setAutoStart(bool autoStart);
    bool loaded() const { return m_loaded; }
    bool respawned() const { return m_respawned; }

    void setCursorSurface(QWaylandSurface *surface, int hotspotX, int hotspotY, wl_client *client);

    void registerSeat(QWaylandSeat *seat);
    void unregisterSeat(QWaylandSeat *seat);

    // setOutputGeometry must be called before QWaylandWindow registerWindow!!!
    QRect outputGeometry() const { return m_outputGeometry; }
    void setOutputGeometry(QRect const &size) { m_outputGeometry = size; }

    virtual void postInit() {}
    virtual WebOSSurfaceItem* createSurfaceItem(QWaylandQuickSurface *surface);
    virtual WebOSInputMethod* createInputMethod();
    virtual WaylandInputMethodManager* createInputMethodManager(WaylandInputMethod *inputMethod);
    virtual WebOSForeign* createWebOSForeign();
    virtual WebOSInputManager* createInputManager();

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

    void windowsChanged();

    void autoStartChanged();
    void loadCompleted();
    void respawnedChanged();

protected:
    virtual void surfaceCreated(QWaylandSurface *surface);

    virtual QWaylandSeat *createSeat() override;
    virtual QWaylandPointer *createPointerDevice(QWaylandSeat *seat) override;
    virtual QWaylandKeyboard *createKeyboardDevice(QWaylandSeat *seat) override;
    virtual QWaylandTouch *createTouchDevice(QWaylandSeat *seat) override;

    bool acquired() { return m_acquired; }
    QHash<QString, CompositorExtension *> extensions() { return m_extensions; }

private:
    // classes
    class EventPreprocessor : public QObject
    {
    public:
        EventPreprocessor(WebOSCoreCompositor*);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

    private:
        WebOSCoreCompositor* m_compositor;
    };
    friend EventPreprocessor;

    // methods
    void checkDaemonFiles();

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
    void onSurfaceMapped(QWaylandSurface *surface, WebOSSurfaceItem *item);
    void onSurfaceUnmapped(QWaylandSurface *surface, WebOSSurfaceItem *item);
    void onSurfaceDestroyed(QWaylandSurface *surface, WebOSSurfaceItem *item);
    void onSurfaceSizeChanged();

private:
    // variables
    // This is kept here for backwards compatibility.. see the deprecated signal
    WebOSSurfaceItem * m_previousFullscreenSurfaceItem;
    WebOSSurfaceItem * m_fullscreenSurfaceItem;
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
    QWaylandWlShell *m_wlShell = nullptr;
#ifdef MULTIINPUT_SUPPORT
    WebOSInputDevice *m_inputDevicePreallocated;
    int m_lastMouseEventFrom;
#endif

    QSharedPointer<WebOSTablet> m_webosTablet;

    bool m_acquired;
    bool m_directRendering;

    QList<WebOSSurfaceItem*> m_surfacesOnUpdate;
    QVector<WebOSCompositorWindow *> m_windows;

    //Global tick counter to get absolute time stamp for recent window model and LRU surface
    quint32 m_fullscreenTick;

    WebOSSurfaceGroupCompositor* m_surfaceGroupCompositor;

    QScopedPointer<WebOSForeign> m_foreign;

    UnixSignalHandler* m_unixSignalHandler;

    CompositorExtension *webOSWindowExtension();

    EventPreprocessor* m_eventPreprocessor;

    bool m_autoStart;
    bool m_loaded;
    bool m_respawned;
    QRect m_outputGeometry;
    QMap<QString, QVector<WebOSCompositorWindow *>> m_clusters;
    bool m_registered;
    ExtensionFlags m_extensionFlags;
};

#endif // WEBOSCORECOMPOSITOR_H
