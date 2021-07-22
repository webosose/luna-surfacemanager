import QtQuick 2.4

Item {
    id: textScrolling2
    width: animationArea.width
    height: animationArea.height
    visible: true

    Text {
        id: sampleTextUpperAlphabet
        color: "black"
        font.pixelSize: 150
        horizontalAlignment: Text.AlignRight
        text: "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    }

    Text {
        id: sampleTextLowerAlphabet
        color: "black"
        font.pixelSize: 150
        horizontalAlignment: Text.AlignRight
        anchors.top: sampleTextUpperAlphabet.bottom
        text: "abcdefghijklmnopqrstuvwxyzabcdefg"
    }

    Text {
        id: sampleTextDigits
        color: "black"
        font.pixelSize: 150
        horizontalAlignment: Text.AlignRight
        anchors.top: sampleTextLowerAlphabet.bottom
        text: "012345678901234567890123456789"
    }

    PropertyAnimation {
        target: sampleTextUpperAlphabet
        properties: "x"
        from: -sampleTextUpperAlphabet.implicitWidth
        to: animationArea.width
        duration: 2500
        running: true
        loops: Animation.Infinite
    }
    PropertyAnimation {
        target: sampleTextLowerAlphabet
        properties: "x"
        from: -sampleTextLowerAlphabet.implicitWidth
        to: animationArea.width
        duration: 5000
        running: true
        loops: Animation.Infinite
    }
    PropertyAnimation {
        target: sampleTextDigits
        properties: "x"
        from: -sampleTextDigits.implicitWidth
        to: animationArea.width
        duration: 10000
        running: true
        loops: Animation.Infinite
    }
}
