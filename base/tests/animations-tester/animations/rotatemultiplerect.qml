import QtQuick 2.4

Item {
    id: rotateMultipleRects
    visible: true

    property int rowCount: 32
    property int colCount: 32

    Column {
        Repeater {
            model: rowCount
            Row {
                Repeater {
                    model: colCount
                    delegate: Rectangle {
                        id: backgroundRect
                        width: 1920 / colCount
                        height: 640 / rowCount
                        color: "red"
                        Rectangle {
                            width: backgroundRect.width * 7 / 8
                            height: backgroundRect.height * 7 / 8
                            anchors.centerIn: parent
                            color: "orange"
                            Rectangle {
                                width: backgroundRect.width * 6 / 8
                                height: backgroundRect.height * 6 / 8
                                anchors.centerIn: parent
                                color: "yellow"
                                Rectangle {
                                    width: backgroundRect.width * 5 / 8
                                    height: backgroundRect.height * 5 / 8
                                    anchors.centerIn: parent
                                    color: "green"
                                    Rectangle {
                                        width: backgroundRect.width * 4 / 8
                                        height: backgroundRect.height * 4 / 8
                                        anchors.centerIn: parent
                                        color: "blue"
                                        Rectangle {
                                            width: backgroundRect.width * 3 / 8
                                            height: backgroundRect.height * 3 / 8
                                            anchors.centerIn: parent
                                            color: "navy"
                                            Rectangle {
                                                width: backgroundRect.width * 2 / 8
                                                height: backgroundRect.height * 2 / 8
                                                anchors.centerIn: parent
                                                color: "purple"
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        SequentialAnimation {
                            id: rotateRects
                            running: true
                            loops: Animation.Infinite
                            RotationAnimation {
                                target: backgroundRect
                                from: 0
                                to: 360
                                duration: 3000
                            }
                        }
                    }
                }
            }
        }
    }
}
