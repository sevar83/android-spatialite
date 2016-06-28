LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    jni_exception.cpp \
	org_spatialite_database_SQLiteCompiledSql.cpp \
	org_spatialite_database_SQLiteDatabase.cpp \
	org_spatialite_database_SQLiteProgram.cpp \
	org_spatialite_database_SQLiteQuery.cpp \
	org_spatialite_database_SQLiteStatement.cpp \
	org_spatialite_CursorWindow.cpp \
	CursorWindow.cpp \
  jniproj.c \
  posix.c \
  exidx_workaround.cpp
#	org_spatialite_database_SQLiteDebug.cpp

LOCAL_STATIC_LIBRARIES := \
    sqlite \
	spatialite \
    freexl

LOCAL_CFLAGS += -DLOG_NDEBUG

# libs from the NDK
LOCAL_LDLIBS += -llog -latomic

LOCAL_LDFLAGS += -fuse-ld=bfd

LOCAL_MODULE:= libandroid_spatialite

include $(BUILD_SHARED_LIBRARY)

$(call import-module,proj.4)
$(call import-module,geos)
$(call import-module,libiconv)
$(call import-module,libxml2)
$(call import-module,liblzma)
$(call import-module,freexl)
$(call import-module,sqlite)
$(call import-module,libspatialite)

# NOTE: iconv is dependency of Spatialite virtual modules like VirtualText, VirtualShape, VirtualXL, etc. 
