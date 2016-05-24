LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# https://github.com/android/platform_external_sqlite/tree/lollipop-release
SQLITE_PATH := platform_external_sqlite-lollipop-release

LOCAL_SRC_FILES := \
					$(SQLITE_PATH)/dist/sqlite3.c \
					$(SQLITE_PATH)/android/sqlite3_android.cpp \
					$(SQLITE_PATH)/android/PhoneNumberUtils.cpp \
					$(SQLITE_PATH)/android/OldPhoneNumberUtils.cpp

LOCAL_STATIC_LIBRARIES := libicui18n_static libicuuc_static libutils liblog android-platform-utils

LOCAL_CFLAGS += \
	-DNDEBUG=1 \
	-DHAVE_USLEEP=1 \
	-DSQLITE_ENABLE_RTREE \
	-DSQLITE_ENABLE_ICU \
	-DSQLITE_HAVE_ISNAN \
	-DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 \
	-DSQLITE_THREADSAFE=2 \
	-DSQLITE_TEMP_STORE=3 \
	-DSQLITE_POWERSAFE_OVERWRITE=1 \
	-DSQLITE_DEFAULT_FILE_FORMAT=4 \
	-DSQLITE_DEFAULT_AUTOVACUUM=1 \
	-DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 \
	-DSQLITE_ENABLE_FTS3 \
	-DSQLITE_ENABLE_FTS3_BACKWARDS \
	-DSQLITE_ENABLE_FTS4 \
	-DSQLITE_OMIT_BUILTIN_TEST \
	-DSQLITE_OMIT_COMPILEOPTION_DIAGS \
	-DSQLITE_DEFAULT_FILE_PERMISSIONS=0600 \
	-DHAVE_SYS_UIO_H \
  -DHAVE_MALLOC_USABLE_SIZE \
  -Dfdatasync=fsync \
  -fvisibility=hidden
#  -DUSE_PREAD64


LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/$(SQLITE_PATH)/android \
        $(LOCAL_PATH)/$(SQLITE_PATH)/dist

LOCAL_MODULE := android-sqlite

LOCAL_EXPORT_C_INCLUDES := \
				$(LOCAL_PATH)/$(SQLITE_PATH)/android \
				$(LOCAL_PATH)/$(SQLITE_PATH)/dist

include $(BUILD_STATIC_LIBRARY)


