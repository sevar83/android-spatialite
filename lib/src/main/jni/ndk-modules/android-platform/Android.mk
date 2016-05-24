LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android-platform-utils
LOCAL_SRC_FILES := dummy.c
LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/platform_system_core/include \
	$(LOCAL_PATH)/platform_frameworks_base/include
LOCAL_EXPORT_LDLIBS := \
	-L$(LOCAL_PATH)/libs/$(TARGET_ARCH_ABI)

include $(BUILD_STATIC_LIBRARY)

