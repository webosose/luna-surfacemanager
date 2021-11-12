import QtQuick 2.4
import QtQuick.Layouts 1.3
import WebOSServices 1.0
import WebOSCompositorBase 1.0

FocusScope {
    id: root
    focus: true

    property string appId: LS.appId
    property string sessionId: LS.sessionManager.sessionId

    property var localeInfo: Settings.subscribe(
        "com.webos.settingsservice", "getSystemSettings",
        {"keys":["localeInfo"]}, false, sessionId) || {}

    property var displayConfig: Settings.subscribe(
        "com.webos.service.config", "getConfigs",
        {"configNames":["com.webos.surfacemanager.displayConfig"]}) || {}

    Text {
        anchors.centerIn: parent
        text: "WebOSServices"
        color: "white"
        font.pixelSize: 128
        style: Text.Outline
        styleColor: "red"
        opacity: 0.3
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 100
        spacing: 10

        Text {
            id: result1
            color: "white"
            wrapMode: Text.WrapAnywhere
            Layout.preferredHeight: (parent.height - parent.spacing * 2) / 4
            Layout.fillWidth: true
            font.pixelSize: 25
            text: "1) ApplicationService: applicationList\n" + root.sessionId + "\n" + applicationService.jsonApplicationList.apps.map((e) => e.id).slice(0, 10).join(", ") + " ..."
        }

        Text {
            id: result2
            color: "white"
            wrapMode: Text.WrapAnywhere
            Layout.preferredHeight: (parent.height - parent.spacing * 2) / 4
            Layout.fillWidth: true
            font.pixelSize: 25
            text: "2) NotificationService: toastList\n" + root.sessionId + "\n" + notificationService.toastList
        }

        Text {
            id: result3
            color: "white"
            wrapMode: Text.WrapAnywhere
            Layout.preferredHeight: (parent.height - parent.spacing * 2) / 4
            Layout.fillWidth: true
            font.pixelSize: 25
            text: "3) SettingsService: com.webos.settingsservice/getSystemSettings localeInfo\n" + root.sessionId + "\n" + JSON.stringify(localeInfo)
        }

        Text {
            id: result4
            color: "white"
            wrapMode: Text.WrapAnywhere
            Layout.preferredHeight: (parent.height - parent.spacing * 2) / 4
            Layout.fillWidth: true
            font.pixelSize: 25
            text: "4) SettingsService: com.webos.service.config/getConfigs displayConfig\nhost\n" + JSON.stringify(displayConfig)
        }
    }

    property Service applicationService: ApplicationManagerService {
        appId: root.appId
        sessionId: root.sessionId

        onConnectedChanged: {
            console.info("Applicationmanager: connected", connected);
            if (connected) {
                subscribeAppLifeEvents();
                subscribeApplicationList();
            }
        }
    }

    property Service notificationService: NotificationService {
        appId: root.appId
        sessionId: root.sessionId
    }
}
