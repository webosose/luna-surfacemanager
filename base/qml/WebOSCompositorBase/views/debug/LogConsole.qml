// Copyright (c) 2014-2018 LG Electronics, Inc.
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

import WebOS.DeveloperTools 1.0
import WebOSCompositorBase 1.0

DebugWindow {
    id: root
    title: "Log Console"

    mainItem: DebugConsole {
        // Returning false will suppress messages.
        // You can use a filter function to manipulate a message.
        // (See debugFilter for example.)
        debugFilter: function (message) {
            // message.color = "white";
            return Settings.local.debug.logFilter.debug;
        }
        warningFilter: Settings.local.debug.logFilter.warning
        criticalFilter: Settings.local.debug.logFilter.critical
        fatalFilter: Settings.local.debug.logFilter.fatal
    }
}
