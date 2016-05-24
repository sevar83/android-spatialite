#include <stdlib.h>
#include <jni.h>

JNIEXPORT jint JNICALL Java_org_Posix_setenv
  (JNIEnv* env, jclass clazz, jstring key, jstring value, jboolean overwrite)
{
    char* k = (char *) (*env)->GetStringUTFChars(env, key, NULL);
    char* v = (char *) (*env)->GetStringUTFChars(env, value, NULL);
    int err = setenv(k, v, overwrite);
    (*env)->ReleaseStringUTFChars(env, key, k);
    (*env)->ReleaseStringUTFChars(env, value, v);
    return err;
}