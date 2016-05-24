LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := freexl

FREEXL_PATH := $(LOCAL_PATH)/freexl

# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# find $(FREEXL_PATH)/ -name "*.cpp" | grep -Ev "tests|doc" | sort | awk '{ print "\t"$1" \\" }'
FREEXL_FILES :=  \
	${FREEXL_PATH}/src/freexl.c	

LOCAL_CFLAGS += -fvisibility=hidden

LOCAL_C_INCLUDES := $(FREEXL_PATH) $(FREEXL_PATH)/headers
LOCAL_EXPORT_C_INCLUDES := $(FREEXL_PATH)/headers

LOCAL_SRC_FILES := $(FREEXL_FILES)

LOCAL_STATIC_LIBRARIES := iconv

include $(BUILD_STATIC_LIBRARY)
