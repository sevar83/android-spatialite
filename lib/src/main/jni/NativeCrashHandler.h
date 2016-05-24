
#ifndef __NATIVE_CRASH_HANDLER__
#define __NATIVE_CRASH_HANDLER__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

void nativeCrashHandler_onLoad(JavaVM* jvm);

void makeNativeCrashReport(const char *reason);

//extern char * (*crash_data_getter)();

void __nativeCrashHandler_stack_call(const char *file, int line);
void __nativeCrashHandler_stack_push(const char *mtd, const char *file, int line);
void __nativeCrashHandler_stack_pop();

#ifndef NDEBUG
#define native_stack_start        __nativeCrashHandler_stack_push(__func__, __FILE__, __LINE__)
#define native_stack_end          __nativeCrashHandler_stack_pop()
#define native_stack_call         __nativeCrashHandler_stack_call(__FILE__, __LINE__)
#else
#define native_stack_start
#define native_stack_end
#define native_stack_call
#endif

#ifdef __cplusplus
}
#endif

#endif // !__NATIVE_CRASH_HANDLER__
