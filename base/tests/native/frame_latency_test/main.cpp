// Copyright (c) 2021 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <QGuiApplication>
#include <QQuickView>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <webosplatform.h>
#include <webospresentationtime.h>

#include "presentationtime.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQuickView view;

    qmlRegisterType<PresentationTime>("PresentationTime", 1, 0, "PresentationTime");

    view.setSource(QUrl("qrc:///frame_latency_test.qml"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.create();

    if (qEnvironmentVariable("QT_WAYLAND_SHELL_INTEGRATION") == "webos") {
        if (qEnvironmentVariableIsSet("DISPLAY_ID")) {
            bool ok = false;
            int displayId = qgetenv("DISPLAY_ID").toInt(&ok);
            if (ok)
                QGuiApplication::platformNativeInterface()->setWindowProperty(view.handle(), "displayAffinity", displayId);
        }
        view.resize(1920, 1080);
        view.show();
    } else {
        view.showFullScreen();
    }

    app.exec();

    return 0;
}
