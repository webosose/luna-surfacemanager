# Copyright (c) 2018-2024 LG Electronics, Inc.
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

WEBOS_COMPOSITOR_PLATFORM_DEFAULT = ""
WEBOS_EGLFS_INTEGRATION_DEFAULT = ""
WEBOS_DRM_FORMAT_DEFAULT = ""
WEBOS_VIRTUAL_DISPLAY_SUPPORT = 0

backend_drm {
    WEBOS_COMPOSITOR_PLATFORM_DEFAULT = eglfs_webos
    WEBOS_EGLFS_INTEGRATION_DEFAULT = $${EGLFS_INTEGRATION}
    WEBOS_DRM_FORMAT_DEFAULT = $${DRM_FORMAT}
}

virtual_display_support {
   WEBOS_VIRTUAL_DISPLAY_SUPPORT = 1
}

defineReplace(replace_envs) {
    in = $${1}
    out = $${2}

    !exists($$in) {
        error($$in does not exist!)
    }

    # Add variables to replace
    variables = WEBOS_INSTALL_DATADIR \
                WEBOS_INSTALL_BINS \
                WEBOS_INSTALL_SYSCONFDIR \
                WEBOS_COMPOSITOR_PLATFORM_DEFAULT \
                WEBOS_EGLFS_INTEGRATION_DEFAULT \
                WEBOS_DRM_FORMAT_DEFAULT \
                WEBOS_VIRTUAL_DISPLAY_SUPPORT

    command = "sed"
    for(var, variables) {
       command += "-e \"s;@$$var@;$$eval($$var);g\""
    }
    command += $$in > $$out

    system($$command)
    system(chmod --reference=$$in $$out)

    QMAKE_DISTCLEAN += $$out
    export(QMAKE_DISTCLEAN)

    return($$out)
}
