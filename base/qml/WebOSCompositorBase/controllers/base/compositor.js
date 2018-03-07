// Copyright (c) 2013-2018 LG Electronics, Inc.
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

/*!
 * Called from when _any_ surface is mapped, i.e. made visible. When this is called
 * the surface model already knows about this surface
 */
function surfaceMapped(item) {
    console.log(item + " " + item.appId + " " + item.type);
    item.objectName = "surface-for-" + (item.appId || "no-app-id");
    if (__allowed_fullscreen(item)) {
        fullscreenRequested(item);
    }
}

/*!
 * Called from when _any_ surface is unmapped, i.e. hidden. This is called before the
 * surface model receives the call for the unmap
 */
function surfaceUnmapped(item) {
    console.log(item + " " + item.appId + " " + item.type);
    __reset_fullscreen_surface(item);
}

/*!
 * Called from when _any_ surface is destroyed. This is called before the
 * surface model receives call for the destruction.
 *
 * NOTE: This function should not rely on the contents of the item as the
 *       is free to delete it at its will
 */
function surfaceDestroyed(item) {
    console.log(item + " " + item.appId + " " + item.type);
    __reset_fullscreen_surface(item);
}

/*!
 * Called when a surface wants to make itself fullscreen.
 * See WebOSSurfaceItem::requestFullscreen and WebOSCompositor::fullscreenRequested
 */

function fullscreenRequested(item) {
    console.log(item + " " + item.appId + " " + compositor.fullscreen);

    if (compositor.fullscreen !== null)
        compositor.fullscreen.fullscreen = false;

    item.state = Qt.WindowFullScreen;
    item.fullscreen = true;
    compositor.fullscreen = item;
}


function surfaceForApplication(appId) {
    var i, item;

    for (i = compositor.surfaceModel.rowCount() - 1; i >= 0; i--) {
        item = compositor.surfaceModel.data(compositor.surfaceModel.index(i, 0, 0));
        if (item.appId === appId)
            return item;
    }
    return null;
}

function closeByAppId(appId) {
    var item = surfaceForApplication(appId)

        if (item && !item.isProxy()) {
            compositor.closeWindowKeepItem(item);
            return true;
        }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Private utility functions only ment to be called with in compositor.js
////////////////////////////////////////////////////////////////////////////////////////////

function __allowed_fullscreen(item) {
    var surfaceType = item.type;
    if (item.isProxy())
        return false;
    if (item.isPartOfGroup()) {
        return false;
    }
    return surfaceType == "_WEBOS_WINDOW_TYPE_CARD" ||
           surfaceType == "_WEBOS_WINDOW_TYPE_RESTRICTED";
}

function __valid_fullscreen_surface() {
    return compositor.fullscreen != undefined;
}

function __reset_fullscreen_surface(item) {
    if (item == compositor.fullscreen) {
        compositor.fullscreen = null;
    }
}
