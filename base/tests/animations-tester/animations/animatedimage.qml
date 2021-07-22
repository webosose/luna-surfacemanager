import QtQuick 2.4

Item {
    id: animatedImage
    width: animationArea.width
    height: animationArea.height
    visible: true

    AnimatedImage {
        id: spinner
        anchors.centerIn: parent
        width: animationArea.width * 1 / 4
        height: animationArea.height * 1 / 2
        fillMode: Image.PreserveAspectFit
        source: Qt.resolvedUrl("../resources/spinner.gif")
        playing: animatedImage.visible
    }
}
