// Copyright (c) 2020 LG Electronics, Inc.
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

function colorByType(item) {
    if (item) {
        switch (item.type) {
        case "_WEBOS_WINDOW_TYPE_CARD":
            return "orchid";
        case "_WEBOS_WINDOW_TYPE_SYSTEM_UI":
            return "deeppink";
        case "_WEBOS_WINDOW_TYPE_OVERLAY":
            return "tomato";
        case "_WEBOS_WINDOW_TYPE_POPUP":
            return "magenta";
        case "_WEBOS_WINDOW_TYPE_RESTRICTED":
            return "purple";
        default:
            return "red";
        }
    }

    return "white";
}
