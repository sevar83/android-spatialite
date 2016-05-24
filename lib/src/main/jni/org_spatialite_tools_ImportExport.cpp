#undef LOG_TAG
#define LOG_TAG "Database"

#include <utils/Log.h>

#include <jni.h>
#include <JNIHelp.h>

#include <sqlite3.h>

#include <freexl.h>

namespace spatialite {

static jobjectArray getExcelWorksheets(JNIEnv* env, jclass clazz, jstring pathString)
{
#ifdef OMIT_FREEXL
  env->ThrowNew(
		  env->FindClass("java/lang/UnsupportedOperationException"),
		  "Must be compiled without OMIT_FREEXL preprocessor definition");
#else
// attempting to build the Worksheet List

  bool invalid = true;
  jobjectArray Worksheets;
  int ret;
  const void *xl_handle;
  char dummy[1024];
  unsigned int count;
  unsigned int idx;
  const char *utf8_string;
  jclass worksheetClass;

  if (pathString == NULL)
	  env->ThrowNew(env->FindClass("java/lang/NullPointerException"), "file path is null");

  utf8_string = env->GetStringUTFChars(pathString, NULL);

// opening the .XLS file [Workbook]
  strcpy(dummy, utf8_string);
  env->ReleaseStringUTFChars(pathString, utf8_string);
  ret = freexl_open_info(dummy, &xl_handle);
  if (ret != FREEXL_OK)
    goto error;
// checking if Password protected
  ret = freexl_get_info(xl_handle, FREEXL_BIFF_PASSWORD, &count);
  if (ret != FREEXL_OK)
    goto error;
  if (count != FREEXL_BIFF_PLAIN)
    goto error;
// Worksheet entries
  ret = freexl_get_info(xl_handle, FREEXL_BIFF_SHEET_COUNT, &count);
  if (ret != FREEXL_OK)
    goto error;
  if (count == 0)
    goto error;

  worksheetClass = env->FindClass("org/spatialite/tools/ExcelWorksheetInfo");
  Worksheets = (jobjectArray)env->NewObjectArray(count, worksheetClass, NULL);
  for (idx = 0; idx < count; idx++)
    {
      // fetching Worksheets entries
      unsigned int rows;
      unsigned short columns;

      jobject worksheet = env->NewObject(worksheetClass, env->GetMethodID(worksheetClass, "<init>", "()V"));

      ret = freexl_get_worksheet_name(xl_handle, idx, &utf8_string);
      if (ret != FREEXL_OK)
        goto error;
      ret = freexl_select_active_worksheet(xl_handle, idx);
      if (ret != FREEXL_OK)
        goto error;
      ret = freexl_worksheet_dimensions(xl_handle, &rows, &columns);
      if (ret != FREEXL_OK)
        goto error;
      if (utf8_string != NULL)
    	env->SetObjectField(worksheet, env->GetFieldID(worksheetClass, "name", "Ljava/lang/String;"), env->NewStringUTF(utf8_string));
      env->SetIntField(worksheet, env->GetFieldID(worksheetClass, "rows", "I"), rows);
      env->SetIntField(worksheet, env->GetFieldID(worksheetClass, "columns", "I"), columns);

      env->SetObjectArrayElement(Worksheets, idx, worksheet);
    }
  invalid = false;
  freexl_close(xl_handle);
  return Worksheets;

error:
  freexl_close(xl_handle);
  if (Worksheets)
    env->DeleteLocalRef(Worksheets);
  return NULL;
#endif /* end FreeXL conditional support */
}

static JNINativeMethod sMethods[] =
{
  /* name, signature, funcPtr */
  {"getExcelWorksheets", "(Ljava/lang/String;)[Lorg/spatialite/tools/ExcelWorksheetInfo;", (void *)getExcelWorksheets}
};

int register_org_spatialite_tools_ImportExport(JNIEnv *env)
{
	jclass clazz;

	clazz = env->FindClass("org/spatialite/tools/ImportExport");
	if (clazz == NULL) {
		LOGE("Can't find org/spatialite/tools/ImportExport\n");
		return -1;
	}

	return jniRegisterNativeMethods(env, "org/spatialite/tools/ImportExport", sMethods, NELEM(sMethods));
}

} // namespace spatialite
