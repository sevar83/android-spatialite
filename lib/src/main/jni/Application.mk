APP_STL := stlport_static
APP_OPTIM := release
APP_ABI := armeabi-v7a,arm64-v8a,x86,x86_64
APP_PLATFORM := android-15
NDK_TOOLCHAIN_VERSION := clang
NDK_APP_LIBS_OUT=../jniLibs
# Temp workaround for https://github.com/android-ndk/ndk/issues/332
APP_DEPRECATED_HEADERS := true