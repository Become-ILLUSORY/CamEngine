/*
 * Android GL 渲染器实现（M2b）
 */
#include "gl_renderer.h"
#include <android/log.h>
#include <GLES2/gl2.h>
#include <cmath>
#include <cstring>

#define GL_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "CamEngineGL", __VA_ARGS__)
#define GL_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "CamEngineGL", __VA_ARGS__)

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x30
#endif

namespace camengine {

namespace {

const char* kVertexSrc = R"(
attribute vec4 aPos;
attribute vec2 aUv;
uniform mat4 uM;
uniform int uRot;
varying vec2 vUv;
void main() {
    vec2 uv = aUv;
    if (uRot == 90)       uv = vec2(uv.y, 1.0 - uv.x);
    else if (uRot == 180) uv = vec2(1.0 - uv.x, 1.0 - uv.y);
    else if (uRot == 270) uv = vec2(1.0 - uv.y, uv.x);
    vUv = (uM * vec4(uv, 0.0, 1.0)).xy;
    gl_Position = aPos;
}
)";

const char* kFragmentSrc = R"(
#extension GL_OES_EGL_image_external : require
precision mediump float;
varying vec2 vUv;
uniform samplerExternalOES uTex;
uniform float uSat;
uniform float uCon;
uniform float uExp;
uniform float uTemp;
uniform float uStr;
void main() {
    vec4 c = texture2D(uTex, vUv);
    vec3 col = c.rgb;
    float luma = dot(col, vec3(0.2126, 0.7152, 0.0722));
    col = mix(vec3(luma), col, uSat);
    col = (col - 0.5) * uCon + 0.5;
    col *= uExp;
    col.r += uTemp * 0.2;
    col.b -= uTemp * 0.2;
    col = mix(c.rgb, col, uStr);
    gl_FragColor = vec4(clamp(col, 0.0, 1.0), c.a);
}
)";

const float kQuadPos[] = {
    -1.f, -1.f, 0.f, 1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f, 0.f, 1.f,
};
const float kQuadUv[] = {
    0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f,
};

GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, sizeof(buf), nullptr, buf);
        GL_LOGE("shader compile fail: %s", buf);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

}  // namespace

GlRenderer::~GlRenderer() {
    release();
}

bool GlRenderer::init() {
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) { GL_LOGE("no display"); return false; }
    EGLint major, minor;
    if (!eglInitialize(display_, &major, &minor)) { GL_LOGE("eglInit fail"); return false; }

    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLint num = 0;
    if (!eglChooseConfig(display_, attribs, &config_, 1, &num) || num < 1) {
        GL_LOGE("eglChooseConfig fail"); return false;
    }

    EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, ctxAttribs);
    if (context_ == EGL_NO_CONTEXT) { GL_LOGE("ctx fail"); return false; }

    EGLint pb[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    pbuffer_ = eglCreatePbufferSurface(display_, config_, pb);
    if (pbuffer_ == EGL_NO_SURFACE) { GL_LOGE("pbuffer fail"); return false; }
    if (!eglMakeCurrent(display_, pbuffer_, pbuffer_, context_)) { GL_LOGE("makeCurrent fail"); return false; }

    glGenTextures(1, &oesTex_);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, oesTex_);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    if (!compileShaders()) return false;

    glGenBuffers(1, &vboPos_);
    glBindBuffer(GL_ARRAY_BUFFER, vboPos_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadPos), kQuadPos, GL_STATIC_DRAW);
    glGenBuffers(1, &vboUv_);
    glBindBuffer(GL_ARRAY_BUFFER, vboUv_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadUv), kQuadUv, GL_STATIC_DRAW);

    GL_LOGI("GL init ok, oesTex=%u", oesTex_);
    return true;
}

bool GlRenderer::compileShaders() {
    GLuint vs = compile(GL_VERTEX_SHADER, kVertexSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFragmentSrc);
    if (!vs || !fs) return false;
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glBindAttribLocation(program_, 0, "aPos");
    glBindAttribLocation(program_, 1, "aUv");
    glLinkProgram(program_);
    GLint ok = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) { GL_LOGE("link fail"); return false; }

    locPos_ = glGetAttribLocation(program_, "aPos");
    locUv_ = glGetAttribLocation(program_, "aUv");
    locTex_ = glGetUniformLocation(program_, "uTex");
    locM_ = glGetUniformLocation(program_, "uM");
    locRot_ = glGetUniformLocation(program_, "uRot");
    locSat_ = glGetUniformLocation(program_, "uSat");
    locCon_ = glGetUniformLocation(program_, "uCon");
    locExp_ = glGetUniformLocation(program_, "uExp");
    locTemp_ = glGetUniformLocation(program_, "uTemp");
    locStr_ = glGetUniformLocation(program_, "uStr");
    return true;
}

bool GlRenderer::setOutputWindow(ANativeWindow* window) {
    if (!display_ || !config_) return false;
    if (outputSurface_ != EGL_NO_SURFACE) {
        eglMakeCurrent(display_, pbuffer_, pbuffer_, context_);
        eglDestroySurface(display_, outputSurface_);
        outputSurface_ = EGL_NO_SURFACE;
    }
    if (window_) { ANativeWindow_release(window_); window_ = nullptr; }
    if (!window) return true;  // 仅清理

    window_ = window;
    ANativeWindow_acquire(window_);
    outputSurface_ = eglCreateWindowSurface(display_, config_, window_, nullptr);
    if (outputSurface_ == EGL_NO_SURFACE) {
        GL_LOGE("eglCreateWindowSurface fail: 0x%x", eglGetError());
        return false;
    }
    return true;
}

bool GlRenderer::makeCurrent() {
    if (!display_ || context_ == EGL_NO_CONTEXT) return false;
    EGLSurface s = (outputSurface_ != EGL_NO_SURFACE) ? outputSurface_ : pbuffer_;
    if (!eglMakeCurrent(display_, s, s, context_)) {
        GL_LOGE("makeCurrent fail: 0x%x", eglGetError());
        return false;
    }
    return true;
}

bool GlRenderer::render(unsigned int oesTex, const float stMatrix[16], int rotationDeg,
                        const GradeParams& p) {
    if (outputSurface_ == EGL_NO_SURFACE) return false;
    EGLint w = 0, h = 0;
    eglQuerySurface(display_, outputSurface_, EGL_WIDTH, &w);
    eglQuerySurface(display_, outputSurface_, EGL_HEIGHT, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, oesTex);
    glUniform1i(locTex_, 0);
    glUniformMatrix4fv(locM_, 1, GL_FALSE, stMatrix);
    glUniform1i(locRot_, rotationDeg);
    glUniform1f(locSat_, p.saturation);
    glUniform1f(locCon_, p.contrast);
    glUniform1f(locExp_, p.exposure);
    glUniform1f(locTemp_, p.temperature);
    glUniform1f(locStr_, p.strength);

    glBindBuffer(GL_ARRAY_BUFFER, vboPos_);
    glEnableVertexAttribArray(locPos_);
    glVertexAttribPointer(locPos_, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, vboUv_);
    glEnableVertexAttribArray(locUv_);
    glVertexAttribPointer(locUv_, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(locPos_);
    glDisableVertexAttribArray(locUv_);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    return true;
}

bool GlRenderer::swapBuffers() {
    if (outputSurface_ == EGL_NO_SURFACE) return false;
    if (!eglSwapBuffers(display_, outputSurface_)) {
        GL_LOGE("swap fail: 0x%x", eglGetError());
        return false;
    }
    return true;
}

void GlRenderer::release() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (outputSurface_ != EGL_NO_SURFACE) eglDestroySurface(display_, outputSurface_);
        if (pbuffer_ != EGL_NO_SURFACE) eglDestroySurface(display_, pbuffer_);
        if (context_ != EGL_NO_CONTEXT) eglDestroyContext(display_, context_);
        eglTerminate(display_);
    }
    if (window_) { ANativeWindow_release(window_); window_ = nullptr; }
    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
    outputSurface_ = EGL_NO_SURFACE;
    pbuffer_ = EGL_NO_SURFACE;
    config_ = nullptr;
    oesTex_ = 0;
    program_ = 0;
    vboPos_ = vboUv_ = 0;
}

}  // namespace camengine
