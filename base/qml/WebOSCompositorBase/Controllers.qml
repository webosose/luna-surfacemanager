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

import QtQuick 2.4
import WebOSCoreCompositor 1.0
import WebOSCompositorBase 1.0
import WebOSCompositor 1.0

Item {
    id: root
    property var views

    AccessControlPolicy {
        id: accessControlPolicyId
        views: root.views
    }

    KeyController {
        id: keyControllerId
        access: accessControlPolicyId
        viewState: viewStateControllerId
        views: root.views
    }

    // Controllers for views
    ViewStateController {
        id: viewStateControllerId
        access: accessControlPolicyId
        keyController: keyControllerId
        views: root.views
    }

    VideoController {
        id: videoControllerId
    }
}
