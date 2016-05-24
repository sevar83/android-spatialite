APP_PROJECT_PATH := $(shell pwd)
APP_ABI := armeabi-v7a armeabi x86
NDK_MODULE_PATH=$(APP_PROJECT_PATH):$(APP_PROJECT_PATH)/src/main/jni/ndk-modules
APP_STL := stlport_static
APP_CPPFLAGS += -frtti -fexceptions
APP_PLATFORM := android-8
#NDK_TOOLCHAIN_VERSION=4.7
