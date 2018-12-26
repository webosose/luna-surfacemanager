// Copyright (c) 2019 LG Electronics, Inc.
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

Item {
    id: root

    // TODO: Is this needed per display? See PLAT-85432.
    Connections {
        target: videooutputdCommunicator
        onSetVideoDisplayWindowRequested: {
            // luna-send -n 1 -f luna://com.webos.service.videooutputd/video/display/setDisplayWindow '{ \
            //     "displayOutput":{"width":960,"height":540,"x":160,"y":0}, \
            //     "context": contextId, \
            //     "fullScreen":false \
            // }'

            var params = JSON.stringify({
                                        "displayOutput": {
                                            "x": destinationRectangle.x,
                                            "y": destinationRectangle.y,
                                            "width": destinationRectangle.width,
                                            "height": destinationRectangle.height
                                        },
                                        "context": contextId,
                                        "fullScreen": false
                                    });

            console.warn("Calling setVideoDisplayWindow:", params);
            LS.adhoc.call("luna://com.webos.service.videooutput", "/video/display/setDisplayWindow", params);
        }
    }
}
