# -------------------
# Android_4.4.0.mk
# [from 'jni/' directory]
# ndk-build clean
# ndk-build
# OR
# ndk-build clean ; ndk-build
# -------------------
# As of 2017-01-18
# -------------------
# changes:
# - geos-3.6.1
# - proj4-3.9.3
# - rttopo 1.1.0-dev
# - json-c-0.12
# - spatialite [4.4.0-20170118]
# -------------------
LOCAL_PATH := $(call my-dir)
JSQLITE_PATH := javasqlite-20120209
SPATIALITE_PATH := libspatialite-4.4.0
GEOS_PATH := geos-3.6.1
JSONC_PATH := json-c-0.12
RTTOPO_PATH := librttopo-20161220
PROJ4_PATH := proj-4.9.3
SQLITE_PATH := sqlite-amalgamation-3081002
ICONV_PATH := libiconv-1.13.1
XML2_PATH := libxml2-2.9.1
LZMA_PATH := xz-5.2.1

include $(LOCAL_PATH)/iconv-1.13.1.mk
include $(LOCAL_PATH)/sqlite-3081002.mk
include $(LOCAL_PATH)/proj4-4.9.3.mk
include $(LOCAL_PATH)/geos-3.6.1.mk
include $(LOCAL_PATH)/json-c-0.12.mk
include $(LOCAL_PATH)/librttopo-1.1.0.mk
include $(LOCAL_PATH)/libxml2-2.9.1.mk
include $(LOCAL_PATH)/lzma-xz-5.2.1.mk
include $(LOCAL_PATH)/spatialite-4.4.0.mk
include $(LOCAL_PATH)/jsqlite-20120209.mk
$(call import-module,android/cpufeatures)
