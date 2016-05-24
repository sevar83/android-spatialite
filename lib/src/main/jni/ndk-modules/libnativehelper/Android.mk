LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

local_src_files := \
    libnativehelper/JNIHelp.cpp \
    libnativehelper/JniConstants.cpp \
    libnativehelper/toStringArray.cpp

LOCAL_SRC_FILES := $(local_src_files)
LOCAL_STATIC_LIBRARIES := liblog_static
LOCAL_MODULE := libnativehelper_static

LOCAL_C_INCLUDES := $(LOCAL_PATH)/libnativehelper/include/nativehelper
#LOCAL_C_INCLUDES := external/stlport/stlport bionic/ bionic/libstdc++/include libcore/include
LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/libnativehelper/include \
	$(LOCAL_PATH)/libnativehelper/include/nativehelper

include $(BUILD_STATIC_LIBRARY)

