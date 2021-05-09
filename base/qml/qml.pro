# Copyright (c) 2017-2021 LG Electronics, Inc.
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

TEMPLATE = aux

include (../config.pri)

# JSON files
json.files = $$replace_envs(settings.json.in, settings.json)
json.path = $$WEBOS_INSTALL_QML/WebOSCompositorBase
INSTALLS += json

use_qresources {
    # Make a qrc file for WebOSCompositorBase
    basedir = $$PWD/WebOSCompositorBase
    baseqrc = $$basedir/WebOSCompositorBase.qrc
    !system(./makeqrc.sh -prefix WebOSCompositorBase $$basedir $$baseqrc): error("Error on running makeqrc.sh")

    # Default qrc
    defaultdir = $$PWD/WebOSCompositorBase/imports/WebOSCompositor
    defaultqrc = $$defaultdir/WebOSCompositorDefault.qrc
    !system(./makeqrc.sh -prefix WebOSCompositor $$defaultdir $$defaultqrc): error("Error on running makeqrc.sh")

    # Install a binary rcc created from qrc files
    basercc = $$PWD/WebOSCompositorBase.rcc
    !system(rcc -binary $$baseqrc $$defaultqrc -o $$basercc): error("Error on running rcc")
    system(rm -f $$baseqrc $$defaultqrc)
    QMAKE_CLEAN += $$basercc

    rcc.files = $$basercc
    rcc.path = $$WEBOS_INSTALL_QML/WebOSCompositorBase
    INSTALLS += rcc
} else {
    # Install WebOSCompositorBase as files
    qml.files = WebOSCompositorBase WebOSCompositor
    qml.path = $$WEBOS_INSTALL_QML
    INSTALLS += qml
}
