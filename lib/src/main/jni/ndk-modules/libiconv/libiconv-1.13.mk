LOCAL_PATH := $(call my-dir)
include ${CLEAR_VARS}

LOCAL_MODULE := iconv

LOCAL_CFLAGS    := \
    -Wno-multichar \
    -D_ANDROID \
    -DLIBDIR "$(LOCAL_PATH)/$(ICONV_PATH)/libcharset/lib" \
    -DBUILDING_LIBICONV \
    -DIN_LIBRARY

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/$(ICONV_PATH) \
    $(LOCAL_PATH)/$(ICONV_PATH)/include \
    $(LOCAL_PATH)/$(ICONV_PATH)/lib \
    $(LOCAL_PATH)/$(ICONV_PATH)/libcharset/include
    
LOCAL_SRC_FILES := \
    $(ICONV_PATH)/lib/iconv.c \
    $(ICONV_PATH)/lib/relocatable.c \
    $(ICONV_PATH)/libcharset/lib/localcharset.c
    
LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/$(ICONV_PATH) \
    $(LOCAL_PATH)/$(ICONV_PATH)/srclib \
    $(LOCAL_PATH)/$(ICONV_PATH)/include

include $(BUILD_STATIC_LIBRARY)
