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

Item {
    id: root
    implicitWidth: _text.implicitWidth
    implicitHeight: _text.implicitHeight

    QtObject {
        id: d
        property font font
        property string text
        property color color: "black"

        // Position alignment
        property int horizontalAlignment: Text.AlignLeft
        readonly property int effectiveHorizontalAlignment: !root.LayoutMirroring.enabled ? horizontalAlignment : _mirrorAligment(horizontalAlignment);
        property int verticalAlignment: Text.AlignTop

        property bool marqueeEnabled: true
        property int marqueeSpeed: 50
        property int marqueeDuration: caculateMarqeeDuration()
        property int marqueeStartGap: 1000
        property int marqueeGap: 1000
    }

    property alias font: d.font
    property alias text: d.text
    property alias color: d.color

    property alias horizontalAlignment: d.horizontalAlignment
    readonly property alias effectiveHorizontalAlignment: d.effectiveHorizontalAlignment
    property alias verticalAlignment: d.verticalAlignment

    property alias marqueeEnabled: d.marqueeEnabled
    property alias marqueeSpeed: d.marqueeSpeed

    function _mirrorAligment(alignment) {
        if (alignment == Text.AlignLeft)
            return Text.AlignRight;
        if (alignment == Text.AlignRight)
            return Text.AlignLeft;
        return alignment;
    }

    // Text direction
    readonly property string _textDirection: _text.horizontalAlignment !== Text.AlignRight ? "LTR" : "RTL";

    property int fullTextWidth: 0
    property int truncatedWidth: root.width

    Item {
        id: clipArea
        x: root.effectiveHorizontalAlignment == Text.AlignRight ? root.width - width :
           root.effectiveHorizontalAlignment == Text.AlignHCenter ? (root.width - width) / 2 : 0
        width: Math.max(truncatedWidth, root.width)
        height: parent.height

        Item {
            x: -parent.x
            width: root.width
            height: root.height

            Text {
                id: _text
                x: horizontalAlignToPosition(root.effectiveHorizontalAlignment)
                y: verticalAlignToPosition(root.verticalAlignment)
                width: (root.width < fullTextWidth) ? root.width : fullTextWidth
                font: root.font
                text: root.text
                color: root.color
                elide: Text.ElideRight

                onContentWidthChanged: {
                    if (truncated)
                        truncatedWidth = _text.contentWidth;
                }

                Component.onCompleted: updateWidthBackups();
            }
        }
    }

    Component.onCompleted: {
        updateState();
    }

    Connections {
        target: root
        function onWidthChanged() {
            forceUpdateStates();
        }
        function onTextChanged() {
            forceUpdateStates();
        }
        function onFontChanged() {
            forceUpdateStates();
        }
        function onMarqueeEnabledChanged() {
            forceUpdateStates();
        }
    }

    onMarqueeSpeedChanged: forceUpdateStates();

    function forceUpdateStates() {
        updateState();
        d.marqueeDuration = caculateMarqeeDuration();
        updateImplicitSize();
        updateWidthBackups();
    }

    function updateState() {
        if (root.marqueeEnabled && (_text.implicitWidth > root.width))
            state = "floating_init";
        else
            state = "normal";
    }

    function caculateMarqeeDuration() {
        return Math.max(((_text.implicitWidth - root.width) / Math.max(root.marqueeSpeed, 1)) * 1000, 0);
    }

    function updateImplicitSize() {
        root.implicitWidth  = _text.implicitWidth;
        root.implicitHeight  = _text.implicitHeight;
    }

    function updateWidthBackups() {
        fullTextWidth = _text.implicitWidth + 1;
        truncatedWidth = _text.contentWidth;
    }

    function horizontalAlignToPosition(align) {
        if (root.effectiveHorizontalAlignment === Text.AlignLeft)
            return 0;
        if (root.effectiveHorizontalAlignment === Text.AlignHCenter)
            return (root.width - _text.width) / 2;
        if (root.effectiveHorizontalAlignment === Text.AlignRight)
            return root.width - _text.width;
        return 0;
    }

    function verticalAlignToPosition(align) {
        if (root.verticalAlignment === Text.AlignTop)
            return 0;
        if (root.verticalAlignment === Text.AlignVCenter)
            return (root.height - _text.height) / 2;
        if (root.verticalAlignment === Text.AlignBottom)
            return root.height - _text.height;
        return 0;
    }

    state: "normal"
    states: [
        State {
            name: "normal"
        },
        State {
            name: "floating_init"
        },
        State {
            name: "floating_start"
        },
        State {
            name: "floating_end"

            PropertyChanges {
                target: _text
                x: _textDirection === "LTR" ? -fullTextWidth + root.width : 0
                width: fullTextWidth
            }

            PropertyChanges {
                target: clipArea
                clip: true
            }
        }
    ]

    transitions: [
        Transition {
            from: "*"
            to: "floating_init"
            SequentialAnimation {
                PropertyAction {
                    target: _text
                    property: "width"
                }
                PropertyAction {
                    target: _text
                    property: "x"
                }
                PropertyAnimation {
                    target: root
                    property: "state"
                    to: "floating_start"
                    duration: 5
                }
            }
        },
        Transition {
            from: "floating_init"
            to: "floating_start"
            PropertyAction {
                target: _text
                property: "width"
            }
            PropertyAnimation{
                target: _text
                property: "x"
            }
            PropertyAnimation {
                target: root
                property: "state"
                to: "floating_end"
                duration: d.marqueeStartGap
            }
        },
        Transition {
            from: "floating_start"
            to: "floating_end"
            SequentialAnimation {
                PropertyAction {
                    target: clipArea
                    property: "clip"
                }
                PropertyAction {
                    target: _text
                    property: "width"
                }
                PropertyAnimation {
                    target: _text
                    property: "x"
                    from: _textDirection === "LTR" ? 0 :-_text.implicitWidth + root.width
                    duration: d.marqueeDuration
                }
                PropertyAnimation {
                    target: root
                    property: "state"
                    to: "floating_start"
                    duration: d.marqueeGap / 2
                }
            }
        },
        Transition {
            from: "floating_end"
            to: "floating_start"
            PropertyAction {
                target: _text
                property: "width"
            }
            PropertyAction {
                target: _text
                property: "x"
            }
            PropertyAnimation {
                target: root
                property: "state"
                to: "floating_end"
                duration: d.marqueeGap / 2
            }
        }
    ]

    Connections {
        target: root
        function onTextChanged() {
            if (root.state != "normal")
                root.state = "floating_init";
        }
    }
}
