// Copyright (c) 2013-2019 LG Electronics, Inc.
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

#ifndef WEBOSSURFACEITEM_H
#define WEBOSSURFACEITEM_H

#include <WebOSCoreCompositor/weboscompositorexport.h>

#include <QObject>
#include <QPointer>
#include <QFlags>
#include <QtCompositor/qwaylandinput.h>

#include <qwaylandsurfaceitem.h>
#include <qwaylandsurface.h>
#include <qwaylandclient.h>
#include <wayland-server.h>

#include <sys/types.h>
#include <unistd.h>

class WebOSCoreCompositor;
class WebOSWindowModel;
class WebOSGroupedWindowModel;
class WebOSShellSurface;

class WebOSSurfaceItem;
class WebOSSurfaceGroup;

/*!
 * \brief Provides a webOS specific surface item for the qml compositor
 *
 * Provides convenient access to the compositor. Currently only
 * fullscreen requests can be made.
 */
class WEBOS_COMPOSITOR_EXPORT WebOSSurfaceItem : public QWaylandSurfaceItem
{
    Q_OBJECT
    Q_FLAGS(WindowClass)
    Q_FLAGS(LocationHints)
    Q_FLAGS(KeyMasks)

    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(QVariantMap windowProperties READ windowProperties NOTIFY windowPropertiesChanged)
    Q_PROPERTY(QString appId READ appId NOTIFY appIdChanged)
    Q_PROPERTY(QString type READ type NOTIFY typeChanged)
    Q_PROPERTY(WindowClass windowClass READ windowClass WRITE setWindowClass NOTIFY windowClassChanged)
    Q_PROPERTY(bool launchLastApp READ launchLastApp NOTIFY launchLastAppChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString subtitle READ subtitle WRITE setSubtitle NOTIFY subtitleChanged)
    Q_PROPERTY(QString params READ params NOTIFY paramsChanged)
    Q_PROPERTY(QString cardSnapShotFilePath READ cardSnapShotFilePath NOTIFY cardSnapShotFilePathChanged)
    Q_PROPERTY(QString customImageFilePath READ customImageFilePath WRITE setCustomImageFilePath NOTIFY customImageFilePathChanged)
    Q_PROPERTY(QString backgroundImageFilePath READ backgroundImageFilePath WRITE setBackgroundImageFilePath NOTIFY backgroundImageFilePathChanged)
    Q_PROPERTY(QString backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QString processId READ processId NOTIFY processIdChanged)
    Q_PROPERTY(qint32 lastFullscreenTick READ lastFullscreenTick NOTIFY lastFullscreenTickChanged)
    Q_PROPERTY(bool transient READ isTransient NOTIFY transientChanged)
    Q_PROPERTY(WebOSSurfaceItem* transientParent READ transientParent NOTIFY transientParentChanged)
    Q_PROPERTY(WebOSWindowModel* transientModel READ transientModel WRITE setTransientModel NOTIFY transientModelChanged)
    Q_PROPERTY(WebOSGroupedWindowModel* groupedWindowModel READ groupedWindowModel WRITE setGroupedWindowModel NOTIFY groupedWindowModelChanged)
    Q_PROPERTY(LocationHints locationHint READ locationHint NOTIFY locationHintChanged)
    Q_PROPERTY(KeyMasks keyMask READ keyMask NOTIFY keyMaskChanged)
    Q_PROPERTY(ItemState itemState READ itemState WRITE setItemState NOTIFY itemStateChanged)
    Q_PROPERTY(QString itemStateReason READ itemStateReason NOTIFY itemStateReasonChanged)
    Q_PROPERTY(QVariantMap closePolicy READ closePolicy WRITE setClosePolicy NOTIFY closePolicyChanged RESET unsetClosePolicy)

    Q_PROPERTY(Qt::WindowState state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(bool notifyPositionToClient READ notifyPositionToClient WRITE setNotifyPositionToClient NOTIFY notifyPositionToClientChanged)
    Q_PROPERTY(bool exposed READ exposed WRITE setExposed NOTIFY exposedChanged)
    Q_PROPERTY(bool hasKeyboardFocus READ hasKeyboardFocus NOTIFY hasKeyboardFocusChanged)
    Q_PROPERTY(bool grabKeyboardFocusOnClick READ grabKeyboardFocusOnClick WRITE setGrabKeyboardFocusOnClick NOTIFY grabKeyboardFocusOnClickChanged)
    Q_PROPERTY(bool launchRequired READ isLaunchRequired WRITE setLaunchRequired NOTIFY launchRequiredChanged)

    Q_PROPERTY(WebOSSurfaceGroup* surfaceGroup READ surfaceGroup NOTIFY surfaceGroupChanged)

public:

    /*!
     * Indicates the window behavior in the compositor.
     * It can be mixed with each other options.
     * 0x0000 WindowClass_Normal        Indicates the item has normal card behavior
     * 0x0001 WindowClass_Hidden        Indicates the item is hidden in the recent
     */
    enum WindowClassFlag {
        WindowClass_Normal        = 0x00,
        WindowClass_Hidden        = 0x01
    };
    Q_DECLARE_FLAGS(WindowClass, WindowClassFlag)

    enum LocationHint {
        LocationHintNorth  = 1,
        LocationHintWest   = 2,
        LocationHintSouth  = 4,
        LocationHintEast   = 8,
        LocationHintCenter = 16
    };
    Q_DECLARE_FLAGS(LocationHints, LocationHint)

    enum KeyMask {
        KeyMaskHome                 = 1,
        KeyMaskBack                 = 1 << 1,
        KeyMaskExit                 = 1 << 2,
        KeyMaskLeft                 = 1 << 3,
        KeyMaskRight                = 1 << 4,
        KeyMaskUp                   = 1 << 5,
        KeyMaskDown                 = 1 << 6,
        KeyMaskOk                   = 1 << 7,
        KeyMaskNumeric              = 1 << 8,
        KeyMaskRemoteColorRed       = 1 << 9,
        KeyMaskRemoteColorGreen     = 1 << 10,
        KeyMaskRemoteColorYellow    = 1 << 11,
        KeyMaskRemoteColorBlue      = 1 << 12,
        KeyMaskRemoteProgrammeGroup = 1 << 13,
        KeyMaskRemotePlaybackGroup  = 1 << 14,
        KeyMaskRemoteTeletextGroup  = 1 << 15,
        KeyMaskLocalLeft            = 1 << 16,
        KeyMaskLocalRight           = 1 << 17,
        KeyMaskLocalUp              = 1 << 18,
        KeyMaskLocalDown            = 1 << 19,
        KeyMaskLocalOk              = 1 << 20,
        KeyMaskRemoteMagnifierGroup = 1 << 21,
        KeyMaskMinimalPlaybackGroup = 1 << 22,
        KeyMaskGuide                = 1 << 23,
        KeyMaskDefault = 0xFFFFFFF8
    };
    Q_DECLARE_FLAGS(KeyMasks, KeyMask)

    enum ItemState {
        ItemStateNormal  = 1,
        ItemStateHidden,
        ItemStateProxy,
        ItemStateClosing,
    };
    Q_ENUM(ItemState)

    WebOSSurfaceItem(WebOSCoreCompositor* compositor, QWaylandQuickSurface* surface);
    ~WebOSSurfaceItem();

    /*!
     * Requests that this surface is minimized.
     * The action is not guaranteed.
     */
    Q_INVOKABLE void requestMinimize();

    /*!
     * Requests that this surface is made fullscreen.
     * The action is not guaranteed.
     */
    Q_INVOKABLE bool requestFullscreen();

    /*!
     * Requests is surface exist or not.
     */
    Q_INVOKABLE bool isSurfaced() const { return surface() && surface()->client(); }

    /*!
     * Return whether the item is proxy or not.
     */
    Q_INVOKABLE bool isProxy() const { return m_itemState == ItemStateProxy; }

    /*!
     * Return whether the item is closing or not.
     */
    Q_INVOKABLE bool isClosing() const { return m_itemState == ItemStateClosing; }

    /*!
     * Returns whether the surface is made fullscreen.
     */
    bool fullscreen() const;

    /*!
     * Override the hover event to dispatch all mouse move events to the surface
     * not just when a mouse button is pressed. The QWaylandSurfaceItem does not
     * handle the hover events at all.
     */
    void hoverMoveEvent(QHoverEvent *event);

    void hoverEnterEvent(QHoverEvent *event);
    void hoverLeaveEvent(QHoverEvent *event);

    void mouseUngrabEvent() Q_DECL_OVERRIDE;

    int texture() const {
        if (textureProvider()->texture())
            return textureProvider()->texture()->textureId();
        return 0;
    }
    QSize textureSize() const { return surface()->size(); }

    void setFullscreen(bool enabled);

    QVariantMap windowProperties();

    void setWindowProperty(const QString& key, const QVariant& value);

    /*!
     * Convenience function to return the appId for this surface.
     */
    QString appId() { return m_appId; }

    /*!
     * Function to set app ID (used for proxy item)
     */
    void setAppId(const QString &appId, bool updateProperty = true);

    /*!
     * Return the type of this surface. If the client did not specify a type
     * _WEBOS_WINDOW_TYPE_CARD will be returned.
     */
    QString type() { return m_type; }

    /*!
     * Function to set type for an item.
     */
    void setType(const QString &type, bool updateProperty = true);

    /*!
     * Return the window class of this surface.
     */
    WindowClass windowClass() { return m_windowClass; }

    void setWindowClass(WindowClass windowClass, bool updateProperty = true);

    /*!
     * Convenience function to return the title for this surface.
     */
    QString title() { return m_title; }

    /*!
     * Function to set title for this surface.
     */
    void setTitle(const QString &title, bool updateProperty = true);

    /*!
     * Convenience function to return the subtitle for this surface.
     */
    QString subtitle() { return m_subtitle; }

    /*!
     * Function to set subtitle for proxy item.
     */
    void setSubtitle(const QString &subtitle, bool updateProperty = true);

    /*!
     * Convenience function to return the close policy for this surface.
     */
    QVariantMap closePolicy() { return m_closePolicy; }

    /*!
     * Function to set close policy for this surface.
     */
    void setClosePolicy(QVariantMap &policy);

    /*!
     * Function to unset close policy for this surface.
     */
    void unsetClosePolicy() { m_closePolicy.clear(); }

    /*!
     * Convenience function to return the _WEBOS_LAUNCH_PREV_APP_AFTER_CLOSING for this surface.
     */
    bool launchLastApp() { return m_launchLastApp; }

    /*!
     * Function to set launch last app flag for an item.
     */
    void setLaunchLastApp(const bool& launchLastApp, bool updateProperty = true);

    /*!
     * Convenience function to return the additional params(json text) for this surface.
     */
    QString params() { return m_params; }

    /*!
     * Function to set type for an item.
     */
    void setParams(const QString &params, bool updateProperty = true);

    /*!
     * Indicates weather this surface is a transient surface to some other
     * surface.
     */
    bool isTransient() const;

    /*!
     * Returns the parent surface for this surface. If there is no
     * parent, i.e. this surface is not a transient surface, NULL
     * is returned.
     */
    WebOSSurfaceItem* transientParent();

    /*!
     * The returned model holds all the transient surfaces for this surface
     */
    WebOSWindowModel* transientModel() { return m_transientModel; }

    WebOSGroupedWindowModel* groupedWindowModel() { return m_groupedWindowModel; }

    /*!
     * Sets the model. Currently the model is created in qml
     */
    void setTransientModel(WebOSWindowModel* model);

    void setGroupedWindowModel(WebOSGroupedWindowModel* model);

    /*!
     * Convenience function to return the processId for this surface.
     */
    QString processId() {
        pid_t pid;

        // Look more info here: https://doc.qt.io/qt-5/qwaylandclient.html
        struct wl_client *client = surface() && surface()->client() ? surface()->client()->client() : NULL;
        if (client)
            wl_client_get_credentials(client, &pid, 0,0);
        else
            pid = getpid();
        m_processId = QString("%1").arg(pid);
        return m_processId;
    }

    /*!
     * Convenience function to return the time since the last fullscreen mode for this surface.
     */
    qint32 lastFullscreenTick() const { return m_lastFullscreenTick; }

    /*!
     * Function to set last fullscreen tick for proxy item.
     */
    void setLastFullscreenTick(quint32 tick) { m_lastFullscreenTick = tick; }

    /*!
     * Function to set path to snapshot.
     */
    Q_INVOKABLE void setCardSnapShotFilePath(const QString& fPath);

    /*!
     * Function to return path to snapshot.
     */
    QString cardSnapShotFilePath()    { return m_cardSnapShotFilePath; }
    QString getCardSnapShotFilePath() { return cardSnapShotFilePath(); }

    /*!
     * Function to get/set custom image path to snapshot.
     */
    QString customImageFilePath() { return  m_customImageFilePath;}
    void setCustomImageFilePath(QString filePath);

    /*!
     * Function to get/set background image path to snapshot.
     */
    QString backgroundImageFilePath() { return  m_backgroundImageFilePath;}
    void setBackgroundImageFilePath(QString filePath);

    /*!
     * Function to get/set background color.
     */
    QString backgroundColor() { return m_backgroundColor; }
    void setBackgroundColor(QString color) { m_backgroundColor = color; }

    /*!
     * Function to delete snapshot.
     */
    void deleteSnapShot();

    /*!
     * Return whether the item has keyboard focus or not.
     */
    bool hasKeyboardFocus() const { return m_hasKeyboardFocus; }

    /*!
     * Return whether the item grabs keyboard focus on mouse click or not.
     */
    bool grabKeyboardFocusOnClick() const { return m_grabKeyboardFocusOnClick; }
    void setGrabKeyboardFocusOnClick(bool grabFocus) { m_grabKeyboardFocusOnClick = grabFocus; }


    void setShellSurface(WebOSShellSurface* shell);
    WebOSShellSurface* shellSurface() { return m_shellSurface; }

    /*!
     * Function to notify that state is about to be changed.
     */
    Q_INVOKABLE void prepareState(Qt::WindowState state);
    void setState(Qt::WindowState state);
    Qt::WindowState state();

    Q_INVOKABLE void close();

    LocationHints locationHint();

    KeyMasks keyMask() const;

    Q_INVOKABLE void resizeClientTo(int width, int height);

    bool notifyPositionToClient() { return m_notifyPositionToClient; }
    bool exposed() { return m_exposed; }

    bool isLaunchRequired() { return m_launchRequired; }
    void setLaunchRequired(bool required);

    ItemState itemState() { return m_itemState; }
    void setItemState(ItemState state, const QString &reason = QString());

    QString itemStateReason() { return m_itemStateReason; }
    void setItemStateReason(const QString &reason);
    void unsetItemStateReason() { m_itemStateReason.clear(); }

    WebOSSurfaceGroup* surfaceGroup() { return m_surfaceGroup; }

    void setSurfaceGroup(WebOSSurfaceGroup* group);

    Q_INVOKABLE bool isPartOfGroup();
    Q_INVOKABLE bool isSurfaceGroupRoot();

    bool acceptsKeyEvent(QKeyEvent *event) const;
    KeyMasks keyMaskFromQt(int key) const;

    bool isMapped();

    void sendCloseToGroupItems();

    void setCursorSurface(QWaylandSurface *surface, int hotSpotX, int hotSpotY);

    void setNotifyPositionToClient(bool notify);

    void setExposed(bool exposed);

public slots:
    void updateScreenPosition();
    void updateProperties(const QVariantMap &properties, const QString &name, const QVariant &value);
    void updateCursor();

signals:
    /*!
     * \brief This signal is emitted when the surface enters or exits fullscreen mode
     */
    void fullscreenChanged(bool enabled);

    /*!
     * \brief Emitted when the underlying window that this surface represents has changed one or more of its properties
     */
    void windowPropertiesChanged(const QVariantMap &windowProperties);


    void transientParentChanged();
    void transientModelChanged();
    void transientChanged();

    void groupedWindowModelChanged();

    void typeChanged();
    void appIdChanged();
    void windowClassChanged();
    void launchLastAppChanged();
    void titleChanged();
    void subtitleChanged();
    void paramsChanged();

    void processIdChanged();
    void lastFullscreenTickChanged();
    void hasKeyboardFocusChanged();
    void grabKeyboardFocusOnClickChanged();

    void cardSnapShotFilePathChanged();

    void dataChanged();

    void locationHintChanged();
    void keyMaskChanged();
    void itemStateChanged();
    void surfaceAboutToBeDestroyed();
    void stateChanged();
    void notifyPositionToClientChanged();
    void exposedChanged();
    void launchRequiredChanged();
    void itemStateReasonChanged();
    void closePolicyChanged();

    void surfaceGroupChanged();

    void zOrderChanged(int zOrder);

    void customImageFilePathChanged();
    void backgroundImageFilePathChanged();

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void touchEvent(QTouchEvent *event);

    virtual bool contains(const QPointF & point) const;

    QPointF mapToTarget(const QPointF& point) const;
    QList<QTouchEvent::TouchPoint> mapToTarget(const QList<QTouchEvent::TouchPoint>& points) const;

    void takeWlKeyboardFocus() const;
    bool isWlKeyboardFocusTaken() const;

    QWaylandInputDevice* getInputDevice(QInputEvent *event) const;

private:
    // methods
    bool getCursorFromSurface(QWaylandSurface *surface, int hotSpotX, int hotSpotY, QCursor& cursor);

private slots:
    void requestStateChange(Qt::WindowState s);
    void onSurfaceDamaged(const QRegion &region);

private:
    // variables
    WebOSCoreCompositor* m_compositor;
    bool m_fullscreen;
    qint32 m_lastFullscreenTick;
    WebOSWindowModel* m_transientModel;
    WebOSGroupedWindowModel* m_groupedWindowModel;
    QString m_cardSnapShotFilePath;
    QString m_customImageFilePath;
    QString m_backgroundImageFilePath;
    QString m_backgroundColor;
    WebOSShellSurface* m_shellSurface;
    ItemState m_itemState;
    QString m_itemStateReason;

    bool m_notifyPositionToClient;

    QString m_appId;
    QString m_type;
    WindowClass m_windowClass;
    QString m_title;
    QString m_subtitle;
    QString m_params;
    bool m_launchLastApp;

    QString m_processId;
    bool m_exposed;
    bool m_launchRequired;
    bool m_hasKeyboardFocus;
    bool m_grabKeyboardFocusOnClick;

    WebOSSurfaceGroup* m_surfaceGroup;

    QVariantMap m_closePolicy;

    QPointer<QWaylandSurface> m_cursorSurface;
    int m_cursorHotSpotX = -1;
    int m_cursorHotSpotY = -1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(WebOSSurfaceItem::WindowClass)
Q_DECLARE_OPERATORS_FOR_FLAGS(WebOSSurfaceItem::LocationHints)
Q_DECLARE_OPERATORS_FOR_FLAGS(WebOSSurfaceItem::KeyMasks)

#endif // WEBOSSURFACEITEM_H
