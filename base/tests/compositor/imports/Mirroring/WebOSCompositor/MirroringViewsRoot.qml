import QtQuick 2.4
import QtQuick.Window 2.2
import WebOSCoreCompositor 1.0
import Eos.Controls 0.1

FocusScope {
    id: root
    focus: true

    ListModel {
        id: sourceScreens

        Component.onCompleted: {
            compositor.loadCompleted.connect(function() {
                for (var i = 0; i < compositor.windows.length; i++) {
                    if (compositor.windows[i].displayId != compositorWindow.displayId) {
                        sourceScreens.append({
                            name: "Display " + compositor.windows[i].displayId,
                            displayId: compositor.windows[i].displayId
                        });
                    }
                }
            });
        }
    }

    function findView(displayId, viewName) {
        var window = compositor.windows[displayId];
        for (var i = 0; i < window.viewsRoot.children.length; i++) {
            var view = window.viewsRoot.children[i];
            if (view.objectName == viewName)
                return view;
        }

        return null;
    }

    Row {
        id: row
        anchors.fill: parent

        Repeater {
            model: sourceScreens
            anchors.fill: parent

            Item {
                id: itemRoot
                width: root.width / sourceScreens.count
                height: root.height

                property var displayId: model.displayId
                property var buttonHeight: 100
                property Item currentView: null

                function onContentChanged() {
                    if (currentView)
                        mirror.sourceItem = currentView.currentItem;
                }

                Rectangle {
                    id: mirrorButton
                    width: itemRoot.width / 2
                    height: buttonHeight
                    color: "coral"

                    property bool activeMirroring: false

                    Text {
                        anchors.centerIn: parent
                        font.pixelSize: 32
                        text: mirrorButton.activeMirroring ? "Tap to stop mirroring" : "Tap to start mirroring"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (mirrorButton.activeMirroring) {
                                mirrorButton.activeMirroring = false;

                                if (itemRoot.currentView)
                                    itemRoot.currentView.contentChanged.disconnect(onContentChanged);

                                mirror.sourceItem = null;
                            } else {
                                var view = findView(itemRoot.displayId, viewListModel.get(viewSelector.selectedIndex).text);
                                if (view && view.currentItem) {
                                    mirrorButton.activeMirroring = true;

                                    itemRoot.currentView = view;
                                    itemRoot.currentView.contentChanged.connect(onContentChanged);

                                    mirror.sourceItem = view.currentItem;
                                } else {
                                    console.warn("failed to find " + viewSelector.selectedIndex + " in display " + itemRoot.displayId);
                                }
                            }
                        }
                    }
                }

                Item {
                    y: buttonHeight
                    width: itemRoot.width
                    height: itemRoot.height - buttonHeight

                    Rectangle {
                        color: "darkblue"
                        anchors.fill: parent
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            font.pixelSize: 64
                            text: model.name
                            color: "white"
                        }
                    }
                }

                SurfaceItemMirror {
                    id: mirror
                    y: buttonHeight
                    width: itemRoot.width
                    height: itemRoot.height - buttonHeight
                    sourceItem: null

                    property bool expanded: false

                    states: [
                        State {
                            name: "expanded"
                            when: mirror.expanded
                            PropertyChanges {
                                target: mirror
                                x: itemRoot.width / 4
                                y: itemRoot.height / 4 + buttonHeight
                                width: itemRoot.width / 2
                                height: itemRoot.height / 2 - buttonHeight
                            }
                        }
                    ]

                    transitions: Transition {
                        reversible: true
                        NumberAnimation {
                            properties: "x, y, width, height"
                            easing.type: Easing.OutCubic
                            duration: 500
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: mirror.expanded = mirror.expanded ? false : true;
                    }
                }

                ListModel {
                    id: viewListModel
                    ListElement {
                        text: "Empty"
                    }
                }

                Item {
                    x: itemRoot.width / 2
                    width: itemRoot.width / 2
                    height: buttonHeight
                    ComboBox {
                        id: viewSelector
                        anchors.fill: parent
                        mustHighlight: true
                        model: viewListModel
                        onSelectedIndexChanged: {
                            if (selectedIndex >= 0 && selectedIndex < viewListModel.count) {
                                if (mirrorButton.activeMirroring) {
                                    var view = findView(itemRoot.displayId, viewListModel.get(selectedIndex).text);
                                    if (view) {
                                        if (itemRoot.currentView)
                                            itemRoot.currentView.contentChanged.disconnect(onContentChanged);

                                        itemRoot.currentView = view;
                                        itemRoot.currentView.contentChanged.connect(onContentChanged);

                                        mirror.sourceItem = view.currentItem;
                                    } else {
                                        console.warn("cannot find view " + viewListModel.get(selectedIndex).text);
                                    }
                                } else {
                                    console.warn("app mirroring is not activated yet");
                                }
                            }
                        }
                    }
                }

                Component.onCompleted: {
                    for (var i = 0; i < compositor.windows.length; i++) {
                        viewListModel.clear();
                        var sourceWindow = compositor.windows[model.displayId];
                        for (var i = 0; i < sourceWindow.viewsRoot.children.length; i++) {
                            var view = sourceWindow.viewsRoot.children[i];
                            if (view.objectName && (view.objectName.indexOf("fullscreen") != -1 || view.objectName.indexOf("home") != -1))
                                viewListModel.append({text: view.objectName});
                            viewSelector.selectedIndex = 0;
                        }
                    }
                }
            }
        }
    }
}
