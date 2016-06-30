APP_PROJECT_PATH := $(shell pwd)
APP_ABI := armeabi-v7a armeabi x86
NDK_MODULE_PATH=$(APP_PROJECT_PATH):$(APP_PROJECT_PATH)/src/main/jni/ndk-modules
APP_STL := c++_static
# Warning: other than android-15 causes java.lang.UnsatisfiedLinkError: Cannot load library: reloc_library[1306]:   793 cannot locate '__ctype_get_mb_cur_max'...
# when running on API 16
APP_PLATFORM := android-15
