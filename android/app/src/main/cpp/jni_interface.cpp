#include <jni.h>
#include <engine_api.h>
#include <android/log.h>
#include <cstdio>

#define TAG "CamEngine-JNI"

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_illusory_camengine_MainActivity_nativeGetEngineVersion(JNIEnv* env, jobject /*thiz*/) {
    EngineVersion v = engine_get_version();
    char buf[64];
    snprintf(buf, sizeof(buf), "%d.%d.%d-%s", v.major, v.minor, v.patch, v.build);
    __android_log_print(ANDROID_LOG_INFO, TAG, "engine version: %s", buf);
    return env->NewStringUTF(buf);
}

}  // extern "C"