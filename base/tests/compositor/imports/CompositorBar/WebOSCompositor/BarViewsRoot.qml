import QtQuick 2.4

FocusScope {
    id: root
    focus: true

    Text {
        anchors.centerIn: parent
        text: "Compositor Bar"
        color: "white"
        font.pixelSize: 128
        style: Text.Outline
        styleColor: "red"
    }
}
