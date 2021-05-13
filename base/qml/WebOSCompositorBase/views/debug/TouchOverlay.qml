// Copyright (c) 2014-2021 LG Electronics, Inc.
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

MultiPointTouchArea {
    id: root
    anchors.fill: parent
    mouseEnabled: false

    onPressed: (touchPoints) => {
        console.log("PRESSED :" + JSON.stringify(touchPoints.map(function (val) {
                    return val.pointId;
                })
            )
        );
        touchPoints.forEach(function (pt) {
            canvas.pointsToDraw[pt.pointId] = {
                "pointId": pt.pointId,
                "x": pt.x, "y": pt.y,
                "previousX":  pt.previousX,
                "previousY":  pt.previousY
            };
        });
        canvas.requestPaint();
    }

    onUpdated: (touchPoints) => {
        console.log("UPDATED :" + JSON.stringify(touchPoints.map(function (val) {
                    return val.pointId;
                })
            )
        );
        touchPoints.forEach(function (pt) {
            canvas.pointsToDraw[pt.pointId] = {
                "pointId": pt.pointId,
                "x": pt.x, "y": pt.y,
                "previousX":  pt.previousX,
                "previousY":  pt.previousY
            };
        });
        canvas.requestPaint();
    }

    onReleased: (touchPoints) => {
        console.log("RELEASED :" + JSON.stringify(touchPoints.map(function (val) {
                    return val.pointId;
                })
            )
        );
        touchPoints.forEach(function (pt) {
            delete canvas.pointsToDraw[pt.pointId];
        });
        canvas.requestPaint();
    }

    Canvas {
        id: canvas
        anchors.fill: parent

        property var pointsToDraw: []
        property var pointsToClear: []

        property real touchSize: 30.0
        property real cleanDelta: 5.0

        onPaint: (region) => {
            var ctx = getContext("2d");
            clearPoints(ctx, pointsToClear);
            drawPoints(ctx, pointsToDraw);
            pointsToClear = pointsToDraw.slice(0);
        }

        function drawPoints(ctx, pts) {
            ctx.save();
            ctx.fillStyle = "yellow";
            pts.forEach(function(pt) {
                console.log("ONPAINT DRAW :" + pt.pointId);
                ctx.fillRect(pt.x, pt.y, touchSize, touchSize);
                ctx.save();
                ctx.fillStyle = "red";
                ctx.font = "20px serif";
                ctx.fillText(pt.pointId, pt.x, pt.y+touchSize)
                ctx.restore();
            });
            ctx.restore();
        }

        function clearPoints(ctx, pts) {
            pts.forEach(function(pt) {
                console.log("ONPAINT CLEAR :" + pt.pointId);
                ctx.clearRect(pt.x-cleanDelta, pt.y-cleanDelta,
                              touchSize + 2*cleanDelta, touchSize + 2*cleanDelta);

                ctx.clearRect(pt.previousX-cleanDelta, pt.previousY-cleanDelta,
                              touchSize + 2*cleanDelta, touchSize + 2*cleanDelta);
            });
            pts = [];
        }
    }
}
