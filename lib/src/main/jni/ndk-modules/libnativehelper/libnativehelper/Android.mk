# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH := $(call my-dir)

local_src_files := \
    JNIHelp.cpp \
    JniConstants.cpp \
    toStringArray.cpp


#
# Build for the target (device).
#

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(local_src_files)
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libnativehelper

LOCAL_C_INCLUDES := external/stlport/stlport bionic/ bionic/libstdc++/include libcore/include
LOCAL_SHARED_LIBRARIES += libstlport

include $(BUILD_SHARED_LIBRARY)


#
# Build for the host.
#

include $(CLEAR_VARS)
LOCAL_MODULE := libnativehelper
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(local_src_files)
LOCAL_C_INCLUDES := libcore/include
LOCAL_SHARED_LIBRARIES := liblog
include $(BUILD_HOST_SHARED_LIBRARY)
