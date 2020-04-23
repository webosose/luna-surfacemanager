import QtQuick 2.4
import QtQuick.Layouts 1.3
import Eos.Window 0.1
import WebOSServices 1.0

WebOSWindow {
    id: root
    title: "QML Service test"
    width: 1920
    height: 1080
    visible: true
    color: "white"
    windowType: "_WEBOS_WINDOW_TYPE_CARD"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 100
        spacing: 10

        Text {
            id: result1
            color: "black"
            wrapMode: Text.WrapAnywhere
            Layout.preferredHeight: (parent.height - parent.spacing * 2) / 4
            Layout.fillWidth: true
            font.pixelSize: 30
        }

        Text {
            id: result2
            color: "black"
            wrapMode: Text.WrapAnywhere
            Layout.preferredHeight: (parent.height - parent.spacing * 2) / 4
            Layout.fillWidth: true
            font.pixelSize: 30
        }

        Text {
            id: result3
            color: "black"
            wrapMode: Text.WrapAnywhere
            Layout.preferredHeight: (parent.height - parent.spacing * 2) / 4
            Layout.fillWidth: true
            font.pixelSize: 30
        }

        Text {
            id: result4
            color: "black"
            wrapMode: Text.WrapAnywhere
            Layout.preferredHeight: (parent.height - parent.spacing * 2) / 4
            Layout.fillWidth: true
            font.pixelSize: 30
        }
    }

    property Service serviceInHost: Service {
        appId: root.appId
        onResponse: {
            if (method == "/signal/registerServerStatus")
                result1.text = "1) registerServerStatus for com.webos.service.config:" + payload;
            else
                result2.text = "2) Call response from com.webos.service.config:" + payload;
        }
    }

    property Service serviceInSession: Service {
        appId: root.appId
        sessionId: params['sessionId']
        onResponse: {
            if (method == "/signal/registerServerStatus")
                result3.text = "3) registerServerStatus for com.webos.settingsservice:" + payload;
            else
                result4.text = "4) Call response from com.webos.settingsservice:" + payload;
        }
    }

    Component.onCompleted: {
        var param;

        serviceInHost.registerServerStatus("com.webos.service.config");

        param = {"configNames":["com.webos.surfacemanager.*"]}
        serviceInHost.call("luna://com.webos.service.config", "/getConfigs", JSON.stringify(param));

        serviceInSession.registerServerStatus("com.webos.settingsservice", true);

        param = {"keys":["country"], "category":"option"}
        serviceInSession.call("luna://com.webos.settingsservice", "/getSystemSettings", JSON.stringify(param));
    }
}
