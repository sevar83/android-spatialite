# file taken and modified from https://github.com/geopaparazzi/libjsqlite-spatialite-android/blob/master/libjsqlite-spatialite-android/spatialite-android-library/jni/lzma-xz-5.1.3a.mk
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
# ./configure --host=arm-linux-androideabi --target=arm-linux-androideabi --enable-static --with-pic --disable-xz --disable-xzdec --disable-lzmainfo --disable-scripts --disable-lzmadec --disable-shared

LOCAL_MODULE    := liblzma

LZMA_PATH := $(LOCAL_PATH)/xz-5.1.3alpha

# -E -g / -g -O2

lzma_flags := \
 -DHAVE_CONFIG_H \
 -g -O2 \
 -std=gnu99

LOCAL_CFLAGS    := \
 $(lzma_flags)

LOCAL_C_INCLUDES := \
 $(LZMA_PATH)/src/common \
 $(LZMA_PATH)/src/liblzma/api \
 $(LZMA_PATH)/src/liblzma/check \
 $(LZMA_PATH)/src/liblzma/common \
 $(LZMA_PATH)/src/liblzma/delta \
 $(LZMA_PATH)/src/liblzma/lz \
 $(LZMA_PATH)/src/liblzma/lzma \
 $(LZMA_PATH)/src/liblzma/rangecoder \
 $(LZMA_PATH)/src/liblzma/simple \
 $(LZMA_PATH)

LOCAL_SRC_FILES := \
 $(LZMA_PATH)/src/common/tuklib_physmem.c \
 $(LZMA_PATH)/src/liblzma/check/check.c \
 $(LZMA_PATH)/src/liblzma/check/crc32_fast.c \
 $(LZMA_PATH)/src/liblzma/check/crc32_table.c \
 $(LZMA_PATH)/src/liblzma/check/crc64_fast.c \
 $(LZMA_PATH)/src/liblzma/check/crc64_table.c \
 $(LZMA_PATH)/src/liblzma/check/sha256.c \
 $(LZMA_PATH)/src/liblzma/common/alone_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/alone_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/auto_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/block_buffer_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/block_buffer_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/block_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/block_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/block_header_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/block_header_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/block_util.c \
 $(LZMA_PATH)/src/liblzma/common/common.c \
 $(LZMA_PATH)/src/liblzma/common/easy_buffer_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/easy_decoder_memusage.c \
 $(LZMA_PATH)/src/liblzma/common/easy_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/easy_encoder_memusage.c \
 $(LZMA_PATH)/src/liblzma/common/easy_preset.c \
 $(LZMA_PATH)/src/liblzma/common/filter_buffer_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/filter_buffer_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/filter_common.c \
 $(LZMA_PATH)/src/liblzma/common/filter_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/filter_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/filter_flags_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/filter_flags_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/hardware_physmem.c \
 $(LZMA_PATH)/src/liblzma/common/index.c \
 $(LZMA_PATH)/src/liblzma/common/index_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/index_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/index_hash.c \
 $(LZMA_PATH)/src/liblzma/common/outqueue.c \
 $(LZMA_PATH)/src/liblzma/common/stream_buffer_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/stream_buffer_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/stream_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/stream_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/stream_encoder_mt.c \
 $(LZMA_PATH)/src/liblzma/common/stream_flags_common.c \
 $(LZMA_PATH)/src/liblzma/common/stream_flags_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/stream_flags_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/vli_decoder.c \
 $(LZMA_PATH)/src/liblzma/common/vli_encoder.c \
 $(LZMA_PATH)/src/liblzma/common/vli_size.c \
 $(LZMA_PATH)/src/liblzma/delta/delta_common.c \
 $(LZMA_PATH)/src/liblzma/delta/delta_decoder.c \
 $(LZMA_PATH)/src/liblzma/delta/delta_encoder.c \
 $(LZMA_PATH)/src/liblzma/lz/lz_decoder.c \
 $(LZMA_PATH)/src/liblzma/lz/lz_encoder.c \
 $(LZMA_PATH)/src/liblzma/lz/lz_encoder_mf.c \
 $(LZMA_PATH)/src/liblzma/lzma/fastpos_table.c \
 $(LZMA_PATH)/src/liblzma/lzma/lzma2_decoder.c \
 $(LZMA_PATH)/src/liblzma/lzma/lzma2_encoder.c \
 $(LZMA_PATH)/src/liblzma/lzma/lzma_decoder.c \
 $(LZMA_PATH)/src/liblzma/lzma/lzma_encoder.c \
 $(LZMA_PATH)/src/liblzma/lzma/lzma_encoder_optimum_fast.c \
 $(LZMA_PATH)/src/liblzma/lzma/lzma_encoder_optimum_normal.c \
 $(LZMA_PATH)/src/liblzma/lzma/lzma_encoder_presets.c \
 $(LZMA_PATH)/src/liblzma/rangecoder/price_table.c \
 $(LZMA_PATH)/src/liblzma/simple/arm.c \
 $(LZMA_PATH)/src/liblzma/simple/armthumb.c \
 $(LZMA_PATH)/src/liblzma/simple/ia64.c \
 $(LZMA_PATH)/src/liblzma/simple/powerpc.c \
 $(LZMA_PATH)/src/liblzma/simple/simple_coder.c \
 $(LZMA_PATH)/src/liblzma/simple/simple_decoder.c \
 $(LZMA_PATH)/src/liblzma/simple/simple_encoder.c \
 $(LZMA_PATH)/src/liblzma/simple/sparc.c \
 $(LZMA_PATH)/src/liblzma/simple/x86.c

LOCAL_EXPORT_C_INCLUDES := $(LZMA_PATH)/src/liblzma/api

include $(BUILD_STATIC_LIBRARY)