// Copyright (c) 2017-2019 LG Electronics, Inc.
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
import WebOSCompositorBase 1.0

import "."
import WebOSCompositor 1.0

// Golden rule : Never put any function body in this file.
// This file is just description for views, controllers, models, services and their relationship(interface)
FocusScope {
    // Do not give an 'id' to avoid from being used as a global variable
    // When you need the 'global variables', consider to design general interface for the modules.

    // Views
    Views {
        id: viewsId
        anchors.fill: parent
    }

    // Controllers
    Controllers {
        id: controllersId
        views: viewsId.views
    }

    // Services
    Services {
        id: servicesId
        views: viewsId.views
        controllers: controllersId
    }
}
