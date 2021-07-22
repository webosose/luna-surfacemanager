import QtQuick 2.4

Item {
    id: textScrolling
    width: animationArea.width
    height: animationArea.height
    visible: true

    Text {
        id: sampleText
        color: "black"
        font.pixelSize: 150
        horizontalAlignment: Text.AlignRight
        text: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" + "\n" +
              "abcdefghijklmnopqrstuvwxyzabcdefg" + "\n" +
              "012345678901234567890123456789"
    }

    SequentialAnimation {
        id: scrollAnimation
        running: true
        loops: Animation.Infinite
        PropertyAnimation {
            target: sampleText
            properties: "x"
            from: -sampleText.implicitWidth
            to: animationArea.width
            duration: 10000
        }
    }
}
