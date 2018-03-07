// Copyright (c) 2017-2018 LG Electronics, Inc.
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

pragma Singleton

import QtQuick 2.4
import WebOSCoreCompositor 1.0

Item {
    id: root
    objectName: "Utils"

    ScreenShot {
        id: screenShot
    }

    function capture(path, target, format) {
        screenShot.path = path;
        screenShot.target = target;
        screenShot.format = format;

        return screenShot.take();
    }

    function capturedSize() {
        return screenShot.size;
    }

    function center(parent, me) {
        return (parent % 2 ? parent + 1 : parent) / 2 - (me % 2 ? me + 1 : me) / 2;
    }

    Component.onCompleted: console.info("Constructed a singleton type:", root);
}
