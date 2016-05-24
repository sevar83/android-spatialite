LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	org_spatialite_database_SQLiteCompiledSql.cpp \
	org_spatialite_database_SQLiteDatabase.cpp \
	org_spatialite_database_SQLiteProgram.cpp \
	org_spatialite_database_SQLiteQuery.cpp \
	org_spatialite_database_SQLiteStatement.cpp \
	org_spatialite_CursorWindow.cpp \
	CursorWindow.cpp \
  jniproj.c \
  org_spatialite_tools_ImportExport.cpp \
  posix.c
#	org_spatialite_database_SQLiteDebug.cpp	

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include

LOCAL_STATIC_LIBRARIES := \
	libicuuc_static \
	libicui18n_static \
	libnativehelper_static \
	android-sqlite \
	spatialite \
  android-platform-utils \
  freexl
  
## this might save us linking against the private android shared libraries like
## libnativehelper.so, libutils.so, libcutils.so, libicuuc, libicui18n.so
#LOCAL_ALLOW_UNDEFINED_SYMBOLS := false

# libs from the NDK
LOCAL_LDLIBS += -ldl -llog

LOCAL_LDLIBS += -landroid_runtime -lbinder -lutils -lcutils

# Needed for org_spatialite_tools_ImportExport.cpp
LOCAL_CPPFLAGS := -fpermissive

LOCAL_MODULE:= libandroid_spatialite

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android-liblog)
$(call import-module,android-platform)
$(call import-module,android-sqlite)
$(call import-module,icu4c)
$(call import-module,libnativehelper)
$(call import-module,proj.4)
$(call import-module,geos)
$(call import-module,libiconv)
$(call import-module,libxml2)
$(call import-module,liblzma)
$(call import-module,freexl)
$(call import-module,libspatialite)

# NOTE: iconv is dependency of Spatialite virtual modules like VirtualText, VirtualShape, VirtualXL, etc. 
