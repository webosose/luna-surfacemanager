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

import QtQuick 2.4

/*!
 * Provides a way to read a JSON formatted settings file from the local
 * file system. The file is expected to contain a root element that is
 * defined by the 'rootElement' property.
 *
 * The 'files' property will indicate the files to be loaded, if a file is
 * not preset it is simply omitted. The files can be relative or absolute.
 * The position of the file property will indicate the priority of the
 * setting value in the case there are properties with same key. The higher
 * the index, the more important the properties will be.
 *
 * The setting are loaded asynchronously and once loaded the 'ready' signal
 * will be emitted.
 *
 * The values will be available via the 'settings' property. The JSON
 * structure is preserved.
 */

Item {
    id: root

    // Since the setting loading is asynchronouos this property will
    // get true once all the settings have been loaded
    property bool ready: false

    property var defaultSettings: undefined
    property var settings: defaultSettings
    property string rootElement: ""
    property var files: []

    // Hide this from the public
    QtObject {
        id: d
        property bool aborting: false
        property var requests: []
        property int count: 0
    }

    Component.onCompleted: {
        reloadAllFiles();
    }

    function reloadAllFiles() {
        d.aborting = true;
        d.requests.forEach(function (req) {
            req.abort();
        });
        d.requests = [];
        d.aborting = false;

        d.count = 0;
        root.settings = Qt.binding(function() { return root.defaultSettings; });

        if (rootElement.length == 0) {
            console.warn("Settings root element undefined, not loading!");
        } else if (root.files.length == 0) {
            console.warn("No files defined for settings, not loading!");
        } else {
            for(var i in root.files) {
                readSettings(root.files[i]);
            }
        }
    }

    // This function will combine two JSON objects recursively.
    // The source values will replace the target values
    function combine(target) {
        var sources = [].slice.call(arguments, 1);
        sources.forEach(function (source) {
            for (var prop in source) {
                if (Array.isArray(source[prop])) {
                   target[prop] = combine([], target[prop], source[prop]);
                } else if (typeof source[prop] == "object") {
                    target[prop] = combine({}, target[prop], source[prop]);
                } else {
                    target[prop] = source[prop];
                }
            }
        });
        return target;
    }

    function signalReady(parsed) {
        if (root.settings == undefined) {
            settings = parsed;
        } else {
            settings = combine({}, settings, parsed);
        }
        // Once we have parsed all the settings file notify that we are ready
        if (d.count++ == root.files.length - 1) {
            root.ready = true;
            d.count = 0;
        }
    }

    function readSettings(file) {
        var xhr = new XMLHttpRequest;
        xhr.open("GET", file);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE && !d.aborting) {
                var result = {};
                if (xhr.responseText.length > 0 ) {
                    var a = JSON.parse(xhr.responseText);
                    result = a[root.rootElement];
                }
                root.signalReady(result);
            }
        }
        d.requests.push(xhr);
        xhr.send();
    }
}
