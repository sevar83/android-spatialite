APP_STL               := c++_static
APP_OPTIM             := release
APP_ABI               := armeabi-v7a arm64-v8a x86_64
APP_PLATFORM          := android-21
NDK_TOOLCHAIN_VERSION := clang
NDK_APP_LIBS_OUT      := ../jniLibs
# Use response files to avoid Windows command-line length limit (Error 87 from llvm-ar)
NDK_APP_SHORT_COMMANDS := true
