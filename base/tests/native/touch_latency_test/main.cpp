#include <QGuiApplication>
#include <QQuickView>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQuickView view;

    view.setSource(QUrl("qrc:///touch_latency_test.qml"));
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
