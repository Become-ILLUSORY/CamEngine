/*
 * GL 渲染 JNI 桥（M2b）
 * 对应 Kotlin: com.illusory.camengine.CameraGlController
 */
#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <cstdint>
#include <cstring>
#include "gl_renderer.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "CamEngineJNI", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "CamEngineJNI", __VA_ARGS__)

using camengine::GlRenderer;
using camengine::GradeParams;

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_illusory_camengine_CameraGlController_nativeInit(JNIEnv* env, jobject thiz) {
    (void)thiz;
    GlRenderer* r = new GlRenderer();
    if (!r->init()) {
        LOGE("GlRenderer init failed");
        delete r;
        return 0;
    }
    return (jlong)(intptr_t)r;
}

JNIEXPORT jint JNICALL
Java_com_illusory_camengine_CameraGlController_nativeGetOesTexture(JNIEnv* env, jobject thiz, jlong h) {
    (void)env; (void)thiz;
    GlRenderer* r = (GlRenderer*)(intptr_t)h;
    return r ? (jint)r->oesTexture() : 0;
}

JNIEXPORT jboolean JNICALL
Java_com_illusory_camengine_CameraGlController_nativeSetOutputSurface(JNIEnv* env, jobject thiz,
                                                                      jlong h, jobject surface) {
    (void)thiz;
    GlRenderer* r = (GlRenderer*)(intptr_t)h;
    if (!r) return JNI_FALSE;
    ANativeWindow* win = surface ? ANativeWindow_fromSurface(env, surface) : nullptr;
    bool ok = r->setOutputWindow(win);
    if (!ok) LOGE("setOutputWindow failed");
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_illusory_camengine_CameraGlController_nativeMakeCurrent(JNIEnv* env, jobject thiz, jlong h) {
    (void)env; (void)thiz;
    GlRenderer* r = (GlRenderer*)(intptr_t)h;
    return (r && r->makeCurrent()) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_illusory_camengine_CameraGlController_nativeRenderFrame(JNIEnv* env, jobject thiz, jlong h,
                                                                 jfloatArray matrix, jint rot,
                                                                 jfloat sat, jfloat con, jfloat exp,
                                                                 jfloat temp, jfloat str) {
    (void)thiz;
    GlRenderer* r = (GlRenderer*)(intptr_t)h;
    if (!r) return JNI_FALSE;
    float m[16];
    env->GetFloatArrayRegion(matrix, 0, 16, m);
    GradeParams p;
    p.saturation = sat;
    p.contrast = con;
    p.exposure = exp;
    p.temperature = temp;
    p.strength = str;
    return r->render(r->oesTexture(), m, rot, p) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_illusory_camengine_CameraGlController_nativeSwap(JNIEnv* env, jobject thiz, jlong h) {
    (void)env; (void)thiz;
    GlRenderer* r = (GlRenderer*)(intptr_t)h;
    return (r && r->swapBuffers()) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_illusory_camengine_CameraGlController_nativeRelease(JNIEnv* env, jobject thiz, jlong h) {
    (void)env; (void)thiz;
    GlRenderer* r = (GlRenderer*)(intptr_t)h;
    if (r) { r->release(); delete r; }
}

}  // extern "C"
