# Copyright (c) 2014-2020 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

TARGET = WebOSCoreCompositor
MODULE_PLUGIN_TYPES = compositorextensions

MOC_DIR = .moc
OBJECTS_DIR = .obj

# For using path macro like MODULE_BASE_OUTDIR
load(qt_build_paths)

QT += qml quick waylandcompositor quick-private core-private waylandcompositor-private weboscompositorextension
CONFIG += debug

DEFINES += QT_COMPOSITOR_QUICK \
           WEBOS_INSTALL_QTPLUGINSDIR=\\\"$$WEBOS_INSTALL_QTPLUGINSDIR\\\"

!no_pmlog {
    CONFIG += link_pkgconfig
    PKGCONFIG += PmLogLib
    DEFINES += USE_PMLOGLIB
}

# TODO: remove this from here
CONFIG += config_xcomposite config_glx

# Input
HEADERS += \
    weboswindowmodel.h \
    webosgroupedwindowmodel.h \
    weboscorecompositor.h \
    weboscompositorwindow.h \
    weboscompositorinterface.h \
    weboscompositorpluginloader.h \
    webosinputmethod.h \
    webossurfacemodel.h \
    webossurfaceitem.h \
    webosscreenshot.h \
    weboskeyfilter.h \
    webosinputdevice.h \
    webosevent.h \
    weboskeyboard.h \
    compositorextensionfactory.h \
    unixsignalhandler.h

SOURCES += \
    weboswindowmodel.cpp \
    webosgroupedwindowmodel.cpp \
    weboscorecompositor.cpp \
    weboscompositorwindow.cpp \
    weboscompositorpluginloader.cpp \
    webosinputmethod.cpp \
    webossurfacemodel.cpp \
    webossurfaceitem.cpp \
    webosscreenshot.cpp \
    weboskeyfilter.cpp \
    webosinputdevice.cpp \
    weboskeyboard.cpp \
    compositorextensionfactory.cpp \
    unixsignalhandler.cpp

use_config {
    # Config loader
    MODULE_DEFINES += USE_CONFIG
    HEADERS += weboscompositorconfig.h
    SOURCES += weboscompositorconfig.cpp
}

!no_multi_input {
    # Multiple input support
    MODULE_DEFINES += MULTIINPUT_SUPPORT
}

!no_upstart {
    # Upstart signaling support
    MODULE_DEFINES += UPSTART_SIGNALING
}

# lttng-ust tracing
# this header has config guards
HEADERS += weboscompositortracer.h

lttng {
    SOURCES += pmtrace_surfacemanager_provider.c
    HEADERS += pmtrace_surfacemanager_provider.h
    LIBS += -ldl -lurcu-bp -llttng-ust
    MODULE_DEFINES += HAS_LTTNG
}

INCLUDEPATH += shell/
include(shell/shell.pri)

INCLUDEPATH += input/
include(input/input.pri)

INCLUDEPATH += surfacegroup/
include(surfacegroup/surfacegroup.pri)

INCLUDEPATH += webosseat/
include(webosseat/webosseat.pri)

INCLUDEPATH += foreign/
include(foreign/webosforeign.pri)

# The module specific defines are added the MODULE_DEFINES variable
# this variable gets processed in qt_module_pris.prf and the content
# gets included into the resulting qt_lib_weboscompositor.pri
#
# NOTE: The loading of the qt_module needs to happen after so that
# variable contains all the needed defines.
DEFINES += $$MODULE_DEFINES
load(qt_module)
