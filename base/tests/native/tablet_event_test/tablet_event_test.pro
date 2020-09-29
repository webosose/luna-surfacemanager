TEMPLATE = app
TARGET = tablet_event_test

QT = core gui quick

SOURCES += \
    main.cpp \
    tabletitem.cpp \

HEADERS += \
    tabletitem.h \

RESOURCES += \
    tablet_event_test.qrc

target.path = $$$$WEBOS_INSTALL_TESTSDIR/luna-surfacemanager

INSTALLS += target
