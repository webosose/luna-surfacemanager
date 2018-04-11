# Copyright (c) 2013-2018 LG Electronics, Inc.
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

TARGET = WebOSCoreCompositorExtension
load(qt_module)

QT += compositor

SOURCES += compositorextension.cpp \
           compositorextensionplugin.cpp

HEADERS += compositorextension.h \
           compositorextensionplugin.h \
           compositorxinput.h \
           compositorxoutput.h \
           compositorxpointer.h \
           compositorwindowclose.h

headers.files = compositorextension.h \
                compositorextensionplugin.h \
                compositorxinput.h \
                compositorxoutput.h \
                compositorxpointer.h \
                compositorwindowclose.h

CONFIG += create_pc create_prl no_install_prl

QMAKE_PKGCONFIG_NAME = weboscompositorextension
QMAKE_PKGCONFIG_DESCRIPTION = WebOS CoreCompositor Extension
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$header.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += target headers
