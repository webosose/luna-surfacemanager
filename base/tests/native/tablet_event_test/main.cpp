#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "tabletitem.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    //register custom qml component
    qmlRegisterType<TabletItem>("TabletItem", 1, 0, "TabletItem");

    QUrl url(QStringLiteral("qrc:/qml/main.qml"));

    QQmlApplicationEngine engine;
    engine.load(url);

    return app.exec();
}
