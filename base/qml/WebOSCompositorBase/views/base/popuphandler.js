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

var popups = null;

/*!
 * Add a popup into the stack. The input parameter should be
 * a PopupSurface as later we need to be able to map a surface
 * item to the popup.
 */
function addPopup(component, view, surfaceItem) {
    var element = component.createObject(view, {"source": surfaceItem});
    if (element == null) {
        return false;
    }

    if (popups == null)
        popups = new Array(0);
    popups.push(element);

    return true;
}

/*!
 * Removes and destroys the popup that represents
 * the 'surfaceItem'
 */
function removePopup(surfaceItem) {
    if (popups) {
        var found = -1;
        for (var i = 0; i < popups.length; i++) {
            if (popups[i].source == surfaceItem) {
                found = i;
                popups[i].source.close();
                popups[i].destroy();
                break;
            }
        }

        if (found != -1)
            popups.splice(found, 1);

        return popups.length;
    }

    return 0;
}

/*!
 * Gives focus to the top item (recently added popup)
 * of the stack.
 */
function giveFocus() {
    if (popups) {
        var count = popups.length;
        if (count > 0) {
            console.log("giving focus to " + popups[count - 1]);
            popups[count - 1].focus = true;
        }
    }
}

/*!
 * Get the current popup(the last popup)'s source of the stack.
 */
function getCurrentPopup() {
    if (popups) {
        var count = popups.length;
        if (count > 0)
            return popups[count - 1].source;
    }
    return null;
}

/*!
 * Close all popup surfaces.
 */
function closeAllPopup() {
    if (popups) {
        for (var i = popups.length - 1; i >= 0; i--)
            popups[i].source.close();
    }
}

/*
function debugPopups() {
    var result = "popups: " + popups.length + "\n";
    for (var i = 0; i < popups.length; i++)
        result += "#" + i + ": " + popups[i] + " [" + popups[i].source.appId + "]\n"
    return result;
}
*/
