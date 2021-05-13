import QtQuick 2.4
import Eos.Window 0.1

WebOSWindow {
    id: rootWebOSWindow
    windowType: "_WEBOS_WINDOW_TYPE_POPUP"
    width: 512
    height: 512
    visible: true
    color: "transparent"
    locationHint: WebOSWindow.LocationHintCenter
    displayAffinity: params["displayAffinity"]

    Rectangle {
        id: box1
        color: "cyan"
        radius: 50
        border.width: 10
        border.color: "red"
        anchors.fill: parent

        Text {
            id: appDetails
            text: "LocationHintCenter"
            font.pixelSize: 50
            color: rootWebOSWindow.active ? "red" : "black"
            anchors.centerIn: parent
            focus: true
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onClicked: (mouse) => {
            switch (rootWebOSWindow.locationHint) {
                case WebOSWindow.LocationHintCenter: appDetails.text = "LocationHintEast"; rootWebOSWindow.locationHint = WebOSWindow.LocationHintEast; break;
                case WebOSWindow.LocationHintEast: appDetails.text = "LocationHintWest"; rootWebOSWindow.locationHint =  WebOSWindow.LocationHintWest; break;
                case WebOSWindow.LocationHintWest: appDetails.text = "LocationHintNorth"; rootWebOSWindow.locationHint = WebOSWindow.LocationHintNorth; break;
                case WebOSWindow.LocationHintNorth: appDetails.text = "LocationHintSouth"; rootWebOSWindow.locationHint = WebOSWindow.LocationHintSouth; break;
                case WebOSWindow.LocationHintSouth: appDetails.text = "LocationHintCenter"; rootWebOSWindow.locationHint = WebOSWindow.LocationHintCenter; break;
            }
        }
    }
}
