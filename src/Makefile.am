##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
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
##########################################################################
lib_LTLIBRARIES = libxraudio-hal.la
libxraudio_hal_la_SOURCES = ctrlm_xraudio_hal_version.h ctrlm_xraudio_hal.c ctrlm_ptt_hal.c ctrlm_ff_hal.c

include_HEADERS = xraudio_hal_config.json

BUILT_SOURCES = ctrlm_xraudio_hal_version.h xraudio_hal_config.h xraudio_hal_config.json
CLEANFILES    = ctrlm_xraudio_hal_version.h xraudio_hal_config.h xraudio_hal_config.json

ctrlm_xraudio_hal_version.h:
	echo "#define CTRLM_XRAUDIO_HAL_VER_MAJOR \"1\"" >$@
	echo "#define CTRLM_XRAUDIO_HAL_VER_MINOR \"`git -C ${srcdir} rev-list --count 4b825dc642cb6eb9a060e54bf8d69288fbee4904..HEAD``git -C ${srcdir} diff --quiet || echo \"++\"`\"" >>$@
	echo "#define CTRLM_XRAUDIO_HAL_BRANCH    \"${GIT_BRANCH}\"" >>$@
	echo "#define CTRLM_XRAUDIO_HAL_COMMIT_ID \"`git -C ${srcdir} log --format="%H" -n 1`\"" >>$@
	echo "#define CTRLM_XRAUDIO_HAL_VERSION (CTRLM_XRAUDIO_HAL_VER_MAJOR \".\" CTRLM_XRAUDIO_HAL_VER_MINOR)" >>$@

xraudio_hal_config.json: xraudio_hal_config_default.json
	python3 "${VSDK_UTILS_JSON_COMBINE}" -i $< -a "${CTRLM_XRAUDIO_HAL_CONFIG_MIC}:mic" -s "${CTRLM_XRAUDIO_HAL_CONFIG_JSON_SUB}" -a "${CTRLM_XRAUDIO_HAL_CONFIG_JSON_ADD}" -o $@

xraudio_hal_config.h: xraudio_hal_config.json
	python3 "${VSDK_UTILS_JSON_TO_HEADER}" -i $< -o $@
