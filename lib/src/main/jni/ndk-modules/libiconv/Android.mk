LOCAL_PATH := $(call my-dir)
include ${CLEAR_VARS}

LOCAL_MODULE := iconv

LOCAL_CFLAGS    := \
    -Wno-multichar \
    -D_ANDROID \
    -DLIBDIR "$(LOCAL_PATH)/libiconv/libcharset/lib" \
    -DBUILDING_LIBICONV \
    -DIN_LIBRARY \
    -I$(LOCAL_PATH)/libiconv/include/ \
    -I$(LOCAL_PATH)/libiconv/lib/ \
    -I$(LOCAL_PATH)/libiconv/ \
    -I$(LOCAL_PATH)/libiconv/libcharset/include/ 

LOCAL_C_INCLUDES := \
    libiconv \
    libiconv/include \
    libiconv/lib \
    libiconv/libcharset/include
    
LOCAL_SRC_FILES := \
    libiconv/lib/iconv.c \
    libiconv/lib/relocatable.c \
    libiconv/libcharset/lib/localcharset.c
    
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libiconv/srclib $(LOCAL_PATH)/libiconv/include $(LOCAL_PATH)/libiconv

include $(BUILD_STATIC_LIBRARY)
