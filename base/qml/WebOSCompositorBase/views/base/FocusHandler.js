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

.pragma library
// sequence of layers containing Items
// [ { layer #0 }, { layer #1 }, ... ]
var focusChain = []

/*!
 * The top item means the last item of the last layer.
 */

function getTopItem() {
    var topLayer = focusChain.slice(-1)[0];
    if (topLayer) {
        return topLayer[topLayer.length - 1];
    }
    return null;
}

/*!
 * Push an item into the layer in the focus chain and give focus to the top item.
 * If another item already existed in the higher layer,
 * the item couldn't take focus.
 */
function requestFocus(item){
    if (typeof item.layerNumber === "undefined") {
        console.warn("layer number of item was not assigned: " + item);
        return;
    }
    console.log("item: " + item + " [" + item.layerNumber + "]");

    var layer = focusChain[item.layerNumber];
    // If there is no layer for the item, create it.
    if (typeof layer === "undefined") {
        layer = [];
        // NOTE: Assigning an element with array index can add 'undefined'
        //       elements implicitly between the last layer and the current one.
        //       These 'undefined' elements should be considered when the
        //       layer would be removed. (cleanupChain)
        focusChain[item.layerNumber] = layer;
    }

    var i = layer.indexOf(item);
    if (i == -1) {
        console.log("item: " + item + " is pushed into focusChain");
        layer.push(item);
    }

    // Give focus to the top item
    var topItem = getTopItem();
    if (topItem)
        topItem.forceActiveFocus();

    console.log(debug());
}

/*!
 * Item signals it gives up focus to the just below item.
 * Remove last occurence of item in the layer and give focus to the top item.
 */
function releaseFocus(item) {
    if (typeof item.layerNumber === "undefined") {
        console.warn("layer number of item was not assigned: " + item);
        return;
    }
    console.log("item: " + item + " [" + item.layerNumber + "]");

    var layer = focusChain[item.layerNumber];
    if (typeof layer === "undefined") {
        console.warn("releasing focus of item that was not in chain: " + item);
        return;
    }

    var i = layer.lastIndexOf(item);
    if (i == -1) {
        console.warn("releasing focus of item that was not in layer:" + item);
        return;
    }

    // Remove item from layer and unset the focus.
    layer.splice(i, 1);
    item.focus = false;

    // The layer became an empty array.
    if (layer.length == 0)
        cleanupChain();

    // Give focus to the top item
    var topItem = getTopItem();
    if (topItem)
        topItem.forceActiveFocus();

    console.log(debug());
}

function cleanupChain() {
    // Remove 'undefined' element or empty array at the tail of the chain
    // in order to get the top item correctly.
    var lastValidIdx = -1;
    for (var i = focusChain.length - 1; i >= 0; i--) {
        var obj = focusChain[i];
        if (typeof obj !== "undefined" && obj.length > 0) {
            lastValidIdx = i;
            break;
        }
    }

    focusChain.splice(lastValidIdx + 1, (focusChain.length - 1) - lastValidIdx);
}

function debug() {
    var result = "focus chain (" + focusChain.length + ") ";
    for (var i = 0; i < focusChain.length; i++) {
        var layer = focusChain[i];
        result += "#" + i + ": " + layer + "; ";
    }
    return result;
}
