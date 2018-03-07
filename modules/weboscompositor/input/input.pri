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

QT += gui-private compositor-private
CONFIG += link_pkgconfig
PKGCONFIG += wayland-webos-server

SOURCES += \
    $$PWD/waylandtextmodelfactory.cpp \
    $$PWD/waylandtextmodel.cpp \
    $$PWD/waylandinputmethodcontext.cpp \
    $$PWD/waylandinputmethod.cpp \
    $$PWD/waylandinputpanel.cpp \
    $$PWD/waylandinputmethodmanager.cpp


HEADERS += \
    $$PWD/waylandtextmodelfactory.h \
    $$PWD/waylandtextmodel.h \
    $$PWD/waylandinputmethodcontext.h \
    $$PWD/waylandinputmethod.h \
    $$PWD/waylandinputpanel.h \
    $$PWD/waylandinputmethodmanager.h
