import QtQuick 2.4
import Eos.Window 0.1
import WebOSServices 1.0

WebOSWindow {
    id: root
    title: "ACG test"
    width: 1920
    height: 1080
    visible: true
    color: "white"
    windowType: "_WEBOS_WINDOW_TYPE_CARD"

    Rectangle {
        id: startButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 10
        color: "gray"
        width: 500
        height: 100

        Text {
            anchors.centerIn: parent
            color: "black"
            font.pixelSize: 32
            text: "Click here or press RIGHT to test!"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: doTest();
        }

        Component.onCompleted: forceActiveFocus();
        Keys.onRightPressed: doTest();
    }

    Text {
        id: result1
        anchors.top: startButton.bottom
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 10
        color: "black"
        font.pixelSize: 24
        text: "1) registerServerStatus for com.webos.surfacemanager:"
    }

    Text {
        id: result2
        anchors.top: result1.bottom
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 10
        color: "black"
        font.pixelSize: 24
        text: "2) Call response from com.webos.surfacemanager/closeByAppId:"
    }

    Text {
        id: result3
        anchors.top: result2.bottom
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 10
        color: "black"
        font.pixelSize: 24
        text: "3) Call response from com.webos.surfacemanager/captureCompositorOutput:"
    }

    Text {
        id: result4
        anchors.top: result3.bottom
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 10
        color: "black"
        font.pixelSize: 24
        text: "4) Call response from com.webos.surfacemanager/getForegroundAppInfo:"
    }

    property Service myService: Service {
        appId: root.appId
        onResponse: {
            if (method == "/signal/registerServerStatus")
                result1.text = "1) registerServerStatus for com.webos.surfacemanager: Received";
            else if (method == "/closeByAppId")
                result2.text = "2) Call response from com.webos.surfacemanager/closeByAppId: Received";
            else if (method == "/captureCompositorOutput")
                result3.text = "3) Call response from com.webos.surfacemanager/captureCompositorOutput: Received";
            else if (method == "/getForegroundAppInfo")
                result4.text = "4) Call response from com.webos.surfacemanager/getForegroundAppInfo: Received";
        }
    }

    function doTest() {
        var param;

        myService.registerServerStatus("com.webos.surfacemanager");

        param = {"id":"bareapp-nonexistent"}
        myService.call("luna://com.webos.surfacemanager", "/closeByAppId", JSON.stringify(param));

        param = {"output":"/tmp/screenshot.png", "format":"PNG"}
        myService.call("luna://com.webos.surfacemanager", "/captureCompositorOutput", JSON.stringify(param));

        myService.call("luna://com.webos.surfacemanager", "/getForegroundAppInfo");
    }
}
