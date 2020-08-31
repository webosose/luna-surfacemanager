import QtQuick 2.4

FocusScope {
    id: root
    focus: true

    Rectangle {
        color: "steelblue"
        anchors.fill: parent

        Text {
            id: textRoot
            anchors.fill: parent
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: "white"
            font.pixelSize: 48
        }

        Component.onCompleted: updateText();
    }

    Connections {
        target: compositorWindow
        onClusterSizeChanged: updateText();
    }

    function updateText() {
        textRoot.text = compositorWindow + "\n\n" +
                        "Cluster Name: " + compositorWindow.clusterName + "\n" +
                        "Cluster Size: " + compositorWindow.clusterSize + "\n" +
                        "Position in cluster: " + compositorWindow.positionInCluster + "\n\n";

        var windows = compositor.windowsInCluster(compositorWindow.clusterName);
        if (windows.length > 0) {
            textRoot.text += "Windows in cluster:\n";
            for (var i = 0; i < windows.length; i++)
                textRoot.text += "[" + i + "] " + windows[i] + "\n";
        } else {
            textRoot.text += "No windows in cluster";
        }
    }
}
