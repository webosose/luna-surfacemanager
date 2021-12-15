// Copyright (c) 2013-2021 LG Electronics, Inc.
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
#include <QtWaylandCompositor/qwaylandseat.h>

#include <qwaylandquickitem.h>
#include <qwaylandsurface.h>
#include <qwaylandsurfacegrabber.h>
#include <qwaylandclient.h>
#include <wayland-server.h>

#include <sys/types.h>
#include <unistd.h>

class WebOSCoreCompositor;
class WebOSWindowModel;
class WebOSGroupedWindowModel;
class WebOSShellSurface;
class WebOSExported;
class WebOSImported;
class WebOSSurfaceItem;
class WebOSSurfaceGroup;
class QWaylandQuickHardwareLayer;

/*!
 * \brief Provides a webOS specific surface item for the qml compositor
 *
 * Provides convenient access to the compositor. Currently only
 * fullscreen requests can be made.
 */
class WEBOS_COMPOSITOR_EXPORT WebOSSurfaceItem : public QWaylandQuickItem
{
    Q_OBJECT
    Q_FLAGS(WindowClass)
    Q_FLAGS(LocationHints)
    Q_FLAGS(KeyMasks)

    Q_PROPERTY(int displayId READ displayId NOTIFY displayIdChanged)
    Q_PROPERTY(int displayAffinity READ displayAffinity NOTIFY displayAffinityChanged)
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
    Q_PROPERTY(QString userId READ userId NOTIFY userIdChanged)
    Q_PROPERTY(qint32 lastFullscreenTick READ lastFullscreenTick NOTIFY lastFullscreenTickChanged)
    Q_PROPERTY(WebOSGroupedWindowModel* groupedWindowModel READ groupedWindowModel WRITE setGroupedWindowModel NOTIFY groupedWindowModelChanged)
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    Q_MOC_INCLUDE("webosgroupedwindowmodel.h")
#endif
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
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    Q_MOC_INCLUDE("webossurfacegroup.h")
#endif
    Q_PROPERTY(QString addon READ addon NOTIFY addonChanged)
    // Filter function that evaluates acceptance of the addon
    Q_PROPERTY(QJSValue addonFilter READ addonFilter WRITE setAddonFilter NOTIFY addonFilterChanged)
    Q_PROPERTY(bool directUpdateOnPlane READ directUpdateOnPlane WRITE setDirectUpdateOnPlane NOTIFY directUpdateOnPlaneChanged)

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
        KeyMaskTeletextActiveGroup  = 1 << 24,
        KeyMaskData                 = 1 << 25,
        KeyMaskInfo                 = 1 << 26,
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

    enum AddonStatus {
        AddonStatusNull,
        AddonStatusLoaded,
        AddonStatusDenied,
        AddonStatusError,
    };
    Q_ENUM(AddonStatus)

    WebOSSurfaceItem(WebOSCoreCompositor* compositor, QWaylandQuickSurface* surface);
    ~WebOSSurfaceItem();

    int displayId() const { return m_displayId; }
    int displayAffinity() const { return m_displayAffinity; }

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
     * not just when a mouse button is pressed. The QWaylandQuickItem does not
     * handle the hover events at all.
     */
    void hoverMoveEvent(QHoverEvent *event) override;

    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

    void mouseUngrabEvent() override;

    GLuint texture() const {
        QSGTexture *texture = textureProvider() ?  textureProvider()->texture() : nullptr;
        if (texture)
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
            return texture->nativeInterface<QNativeInterface::QSGOpenGLTexture>()->nativeTexture();
#else
            return texture->textureId();
#endif
        return 0;
    }

    void setDisplayAffinity(int affinity);

    void setFullscreen(bool enabled);

    QVariantMap windowProperties();

    // Does not exist anymore since 5.7
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

    WebOSGroupedWindowModel* groupedWindowModel() { return m_groupedWindowModel; }

    void setGroupedWindowModel(WebOSGroupedWindowModel* model);

    /*!
     * Convenience function to return the processId for this surface.
     */
    QString processId() const { return m_processId; }

    /*!
     * Convenience function to return the userId for this surface.
     */
    QString userId() const { return m_userId; }

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
    void resetShellSurface(WebOSShellSurface *shell);
    WebOSShellSurface* shellSurface() const { return m_shellSurface; }

    /*!
     * Function to notify that state is about to be changed.
     */
    Q_INVOKABLE void prepareState(Qt::WindowState state);
    void setState(Qt::WindowState state);
    Qt::WindowState state();

    Q_INVOKABLE void close();

    LocationHints locationHint();

    KeyMasks keyMask() const;

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

    WebOSSurfaceItem *createMirrorItem();
    bool removeMirrorItem(WebOSSurfaceItem *mirror);
    QVector<WebOSSurfaceItem *> mirrorItems() { return m_mirrorItems; }
    Q_INVOKABLE bool isMirrorItem() const { return m_isMirrorItem; }
    WebOSSurfaceItem *mirrorSource() const { return m_mirrorSource; };

    QVector<WebOSExported *> exportedElements() { return m_exportedElements; }
    void appendExported(WebOSExported *exported) { if (!m_exportedElements.contains(exported)) m_exportedElements.append(exported); }
    void removeExported(WebOSExported *exported) { m_exportedElements.removeOne(exported); }

    bool imported() { return m_imported; }
    void setImported(bool imported) { m_imported = imported; }

    virtual bool hasSecuredContent();

    QString addon() const;
    Q_INVOKABLE void setAddonStatus(AddonStatus addonStatus);

    QJSValue addonFilter() const;
    void setAddonFilter(const QJSValue &filter);
    bool acceptsAddon(const QString &newAddon);

    Q_INVOKABLE void grabLastFrame();
    Q_INVOKABLE void releaseLastFrame();

    uint32_t planeZpos () const;
    bool directUpdateOnPlane() const;
    void setDirectUpdateOnPlane(bool enable);

    static WebOSSurfaceItem *getSurfaceItemFromSurface(QWaylandSurface *surface) {
        return (!surface || surface->views().isEmpty()) ? nullptr : qobject_cast<WebOSSurfaceItem*>(surface->views().first()->renderObject());
    }

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
#endif

public slots:
    void updateScreenPosition();
    void updateProperties(const QVariantMap &properties, const QString &name, const QVariant &value);
    void updateCursor();
    void updateDirectUpdateOnPlane();

signals:
    void displayIdChanged();
    void displayAffinityChanged();

    /*!
     * \brief This signal is emitted when the surface enters or exits fullscreen mode
     */
    void fullscreenChanged(bool enabled);

    /*!
     * \brief Emitted when the underlying window that this surface represents has changed one or more of its properties
     */
    void windowPropertiesChanged(const QVariantMap &windowProperties);

    void groupedWindowModelChanged();

    void typeChanged();
    void appIdChanged();
    void windowClassChanged();
    void launchLastAppChanged();
    void titleChanged();
    void subtitleChanged();
    void paramsChanged();

    void processIdChanged();
    void userIdChanged();
    void lastFullscreenTickChanged();
    void hasKeyboardFocusChanged();
    void grabKeyboardFocusOnClickChanged();

    void cardSnapShotFilePathChanged();

    void dataChanged();

    void locationHintChanged();
    void keyMaskChanged();
    void itemStateChanged();
    void itemAboutToBeDestroyed();
    void itemAboutToBeHidden();
    void stateChanged();
    void notifyPositionToClientChanged();
    void exposedChanged();
    void launchRequiredChanged();
    void itemStateReasonChanged();
    void closePolicyChanged();
    void positionUpdated();

    void surfaceGroupChanged();

    void zOrderChanged(int zOrder);

    void customImageFilePathChanged();
    void backgroundImageFilePathChanged();

    void addonChanged();
    void addonFilterChanged();
    void directUpdateOnPlaneChanged();

protected:
    void processKeyEvent(QKeyEvent *event);
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
    virtual void focusInEvent(QFocusEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void touchEvent(QTouchEvent *event) override;

    virtual bool contains(const QPointF & point) const override;

    QList<QTouchEvent::TouchPoint> mapToTarget(const QList<QTouchEvent::TouchPoint>& points) const;

    void takeWlKeyboardFocus() const;

    QWaylandSeat* getInputDevice(QInputEvent *event = nullptr) const;
    void takeFocus(QWaylandSeat *device = nullptr) override;

    void surfaceChangedEvent(QWaylandSurface *newSurface, QWaylandSurface *oldSurface) override;

    bool event(QEvent *) override;
    bool tabletEvent(QTabletEvent* event);

private:
    // methods
    void setDisplayId(int id);
    bool getCursorFromSurface(QWaylandSurface *surface, int hotSpotX, int hotSpotY, QCursor& cursor);

private slots:
    void handleWindowChanged();
    void requestStateChange(Qt::WindowState s);
    void onSurfaceDamaged(const QRegion &region);

private:
    // variables
    WebOSCoreCompositor* m_compositor;
    bool m_fullscreen;
    qint32 m_lastFullscreenTick;
    WebOSGroupedWindowModel* m_groupedWindowModel;
    QString m_cardSnapShotFilePath;
    QString m_customImageFilePath;
    QString m_backgroundImageFilePath;
    QString m_backgroundColor;
    WebOSShellSurface* m_shellSurface;
    ItemState m_itemState;
    QString m_itemStateReason;

    bool m_notifyPositionToClient;
    QPointF m_position;

    int m_displayId;
    int m_displayAffinity;

    QString m_appId;
    QString m_type;
    WindowClass m_windowClass;
    QString m_title;
    QString m_subtitle;
    QString m_params;
    bool m_launchLastApp;

    QString m_processId;
    QString m_userId;
    bool m_exposed;
    bool m_launchRequired;
    bool m_hasKeyboardFocus;
    bool m_grabKeyboardFocusOnClick;

    WebOSSurfaceGroup* m_surfaceGroup;

    QVariantMap m_closePolicy;

    int m_cursorHotSpotX = -1;
    int m_cursorHotSpotY = -1;

    bool m_isMirrorItem = false;
    WebOSSurfaceItem *m_mirrorSource = nullptr;
    QVector<WebOSSurfaceItem *> m_mirrorItems;
    QVector<WebOSExported *> m_exportedElements;
    bool m_imported = false;
    QWaylandView m_cursorView;
    QJSValue m_addonFilter;

    QWaylandSurface *m_surfaceGrabbed = nullptr;
    bool m_directUpdateOnPlane = false;
    QWaylandQuickHardwareLayer *m_hardwarelayer = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(WebOSSurfaceItem::WindowClass)
Q_DECLARE_OPERATORS_FOR_FLAGS(WebOSSurfaceItem::LocationHints)
Q_DECLARE_OPERATORS_FOR_FLAGS(WebOSSurfaceItem::KeyMasks)

#endif // WEBOSSURFACEITEM_H
