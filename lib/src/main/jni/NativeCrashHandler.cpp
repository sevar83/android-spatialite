
#include "NativeCrashHandler.h"

#include <jni.h>
#include <android/log.h>

#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>

#include <dlfcn.h>

#ifndef NDEBUG
#define Verify(x, r)  assert((x) && r)
#else
#define Verify(x, r)  ((void)(x))
#endif


extern "C" {

static struct sigaction old_sa[NSIG];

static jclass applicationClass = 0;
static jmethodID makeCrashReportMethod;
static jobject applicationObject = 0;

static jclass stackTraceElementClass = 0;
static jmethodID stackTraceElementMethod;

static JavaVM *javaVM;


typedef struct map_info_t map_info_t;
typedef struct {
    uintptr_t absolute_pc;
    uintptr_t stack_top;
    size_t stack_size;
} backtrace_frame_t;
typedef struct {
    uintptr_t relative_pc;
    uintptr_t relative_symbol_addr;
    char* map_name;
    char* symbol_name;
    char* demangled_name;
} backtrace_symbol_t;
typedef ssize_t (*t_unwind_backtrace_signal_arch)(siginfo_t* si, void* sc, const map_info_t* lst, backtrace_frame_t* bt, size_t ignore_depth, size_t max_depth);
static t_unwind_backtrace_signal_arch unwind_backtrace_signal_arch;
typedef map_info_t* (*t_acquire_my_map_info_list)();
static t_acquire_my_map_info_list acquire_my_map_info_list;
typedef void (*t_release_my_map_info_list)(map_info_t* milist);
static t_release_my_map_info_list release_my_map_info_list;
typedef void (*t_get_backtrace_symbols)(const backtrace_frame_t* backtrace, size_t frames, backtrace_symbol_t* symbols);
static t_get_backtrace_symbols get_backtrace_symbols;
typedef void (*t_free_backtrace_symbols)(backtrace_symbol_t* symbols, size_t frames);
static t_free_backtrace_symbols free_backtrace_symbols;


void _makeNativeCrashReport(const char *reason, struct siginfo *siginfo, void *sigcontext) {
	JNIEnv *env = 0;

	int result = javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);

	if (result == JNI_EDETACHED) {
		__android_log_print(ANDROID_LOG_WARN, "NativeCrashHandler", "Native crash occured in a non jvm-attached thread");
		result = javaVM->AttachCurrentThread(&env, NULL);
	}

	if (result != JNI_OK)
		__android_log_print(ANDROID_LOG_ERROR, "NativeCrashHandler",
				"Could not attach thread to Java VM for crash reporting.\n"
				"Crash was: %s",
				reason
		);
	else if (env && applicationObject) {
		jobjectArray elements = NULL;

		if (unwind_backtrace_signal_arch != NULL && siginfo != NULL)  {
			map_info_t *map_info = acquire_my_map_info_list();
			backtrace_frame_t frames[256] = {0,};
			backtrace_symbol_t symbols[256] = {0,};
			const ssize_t size = unwind_backtrace_signal_arch(siginfo, sigcontext, map_info, frames, 1, 255);
			get_backtrace_symbols(frames,  size, symbols);

			elements = env->NewObjectArray(size, stackTraceElementClass, NULL);
			Verify(elements, "Could not create StackElement java array");
			int pos = 0;
			jstring jni = env->NewStringUTF("<JNI>");
			for (int i = 0; i < size; ++i) {
				const char *method = symbols[i].demangled_name;
				if (!method)
					method = symbols[i].symbol_name;
				if (!method)
					method = "?";
				//__android_log_print(ANDROID_LOG_ERROR, "DUMP", "%s", method);
				const char *file = symbols[i].map_name;
				if (!file)
					file = "-";
				jobject element = env->NewObject(stackTraceElementClass, stackTraceElementMethod,
						jni,
						env->NewStringUTF(method),
						env->NewStringUTF(file),
						-2
				);
				Verify(element, "Could not create StackElement java object");
				env->SetObjectArrayElement(elements, pos++, element);
				Verify(env->ExceptionCheck() == JNI_FALSE, "Java threw an exception");
			}

			free_backtrace_symbols(symbols, size);
			release_my_map_info_list(map_info);
		}

		env->CallVoidMethod(applicationObject, makeCrashReportMethod, env->NewStringUTF(reason), elements, (jint)gettid());
		Verify(env->ExceptionCheck() == JNI_FALSE, "Java threw an exception");
	}
	else
		__android_log_print(ANDROID_LOG_ERROR, "NativeCrashHandler",
				"Could not create native crash report as registerForNativeCrash was not called in JAVA context.\n"
				"Crash was: %s",
				reason
		);
}

void makeNativeCrashReport(const char *reason) {
	_makeNativeCrashReport(reason, NULL, NULL);
}

void nativeCrashHandler_sigaction(int signal, struct siginfo *siginfo, void *sigcontext) {

	if (old_sa[signal].sa_handler)
		old_sa[signal].sa_handler(signal);

	_makeNativeCrashReport(strsignal(signal), siginfo, sigcontext);
}

JNIEXPORT jboolean JNICALL Java_com_github_nativehandler_NativeCrashHandler_nRegisterForNativeCrash(JNIEnv * env, jobject obj) {

	if (applicationClass) {
		applicationObject = (jobject)env->NewGlobalRef(obj);
		Verify(applicationObject, "Could not create NativeCrashHandler java object global reference");
		return 1;
	}

	return 0;
}

JNIEXPORT void JNICALL Java_com_github_nativehandler_NativeCrashHandler_nUnregisterForNativeCrash(JNIEnv *env, jobject) {
	if (applicationObject) {
		env->DeleteGlobalRef(applicationObject);
	}
}

void nativeCrashHandler_onLoad(JavaVM *jvm) {
	javaVM = jvm;

	JNIEnv *env = 0;
	int result = jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
	Verify(result == JNI_OK, "Could not get JNI environment");

	applicationClass = env->FindClass("com/github/nativehandler/NativeCrashHandler");
	Verify(applicationClass, "Could not find NativeCrashHandler java class");
	applicationClass = (jclass)env->NewGlobalRef(applicationClass);
	Verify(applicationClass, "Could not create NativeCrashHandler java class global reference");
	makeCrashReportMethod = env->GetMethodID(applicationClass, "makeCrashReport", "(Ljava/lang/String;[Ljava/lang/StackTraceElement;I)V");
	Verify(makeCrashReportMethod, "Could not find makeCrashReport java method");

	stackTraceElementClass = env->FindClass("java/lang/StackTraceElement");
	Verify(stackTraceElementClass, "Could not find StackTraceElement java class");
	stackTraceElementClass = (jclass)env->NewGlobalRef(stackTraceElementClass);
	Verify(stackTraceElementClass, "Could not create StackTraceElement java class global reference");
	stackTraceElementMethod = env->GetMethodID(stackTraceElementClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
	Verify(stackTraceElementMethod, "Could not find StackTraceElement constructor java method");

	Verify(env->ExceptionCheck() == JNI_FALSE, "Java threw an exception");

/*	void * libcorkscrew = dlopen("libcorkscrew.so", RTLD_LAZY | RTLD_LOCAL);
	if (libcorkscrew) {
		unwind_backtrace_signal_arch = (t_unwind_backtrace_signal_arch) dlsym(libcorkscrew, "unwind_backtrace_signal_arch");
		acquire_my_map_info_list = (t_acquire_my_map_info_list) dlsym(libcorkscrew, "acquire_my_map_info_list");
		release_my_map_info_list = (t_release_my_map_info_list) dlsym(libcorkscrew, "release_my_map_info_list");
		get_backtrace_symbols  = (t_get_backtrace_symbols) dlsym(libcorkscrew, "get_backtrace_symbols");
		free_backtrace_symbols = (t_free_backtrace_symbols) dlsym(libcorkscrew, "free_backtrace_symbols");
	}*/

	struct sigaction handler;
	memset(&handler, 0, sizeof(handler));
	sigemptyset(&handler.sa_mask);
	handler.sa_sigaction = nativeCrashHandler_sigaction;
	handler.sa_flags = SA_SIGINFO | SA_ONSTACK;

	stack_t stack;
	memset(&stack, 0, sizeof(stack));
	stack.ss_size = 1024 * 128;
	stack.ss_sp = malloc(stack.ss_size);
	Verify(stack.ss_sp, "Could not allocate signal alternative stack");
	stack.ss_flags = 0;
	result = sigaltstack(&stack, NULL);
	Verify(!result, "Could not set signal stack");

	result = sigaction(SIGILL,    &handler, &old_sa[SIGILL]    );
	Verify(!result, "Could not register signal callback for SIGILL");

	result = sigaction(SIGABRT,   &handler, &old_sa[SIGABRT]   );
	Verify(!result, "Could not register signal callback for SIGABRT");

	result = sigaction(SIGBUS,    &handler, &old_sa[SIGBUS]    );
	Verify(!result, "Could not register signal callback for SIGBUS");

	result = sigaction(SIGFPE,    &handler, &old_sa[SIGFPE]    );
	Verify(!result, "Could not register signal callback for SIGFPE");

	result = sigaction(SIGSEGV,   &handler, &old_sa[SIGSEGV]   );
	Verify(!result, "Could not register signal callback for SIGSEGV");

	result = sigaction(SIGSTKFLT, &handler, &old_sa[SIGSTKFLT] );
	Verify(!result, "Could not register signal callback for SIGSTKFLT");

	result = sigaction(SIGPIPE,   &handler, &old_sa[SIGPIPE]   );
	Verify(!result, "Could not register signal callback for SIGPIPE");
}

}
