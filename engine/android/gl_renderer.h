/*
 * Android GL 渲染器（M2b）
 * 职责：持有 EGL 上下文，采样相机 OES 纹理，应用实时调色，
 *       输出到 Flutter 的 SurfaceTexture。
 * 放在 engine/android/ 因为 EGL 是 Android 特有；
 * 调色数学与 shaders/grade.frag 保持一致。
 */
#ifndef CAMENGINE_GL_RENDERER_H_
#define CAMENGINE_GL_RENDERER_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/native_window.h>

namespace camengine {

struct GradeParams {
    float saturation = 1.4f;
    float contrast = 1.15f;
    float exposure = 1.1f;
    float temperature = 0.15f;
    float strength = 0.8f;
};

class GlRenderer {
public:
    GlRenderer() = default;
    ~GlRenderer();

    GlRenderer(const GlRenderer&) = delete;
    GlRenderer& operator=(const GlRenderer&) = delete;

    /* 创建 EGL 上下文 + 离屏 PBuffer + OES 纹理。成功返回 true */
    bool init();
    /* 绑定输出窗口（来自 Flutter SurfaceTexture 的 ANativeWindow） */
    bool setOutputWindow(ANativeWindow* window);
    /* 当前线程设为渲染线程，绑定上下文 */
    bool makeCurrent();
    /* 渲染一帧：oesTex 为相机纹理，stMatrix 为 SurfaceTexture 变换矩阵 */
    bool render(unsigned int oesTex, const float stMatrix[16], int rotationDeg,
                const GradeParams& p);
    bool swapBuffers();
    void release();

    unsigned int oesTexture() const { return oesTex_; }
    bool ready() const { return context_ != EGL_NO_CONTEXT && outputSurface_ != EGL_NO_SURFACE; }

private:
    bool compileShaders();

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLConfig config_ = nullptr;
    EGLSurface pbuffer_ = EGL_NO_SURFACE;
    EGLSurface outputSurface_ = EGL_NO_SURFACE;
    ANativeWindow* window_ = nullptr;

    unsigned int oesTex_ = 0;
    unsigned int program_ = 0;
    unsigned int vboPos_ = 0, vboUv_ = 0;

    GLint locPos_ = -1, locUv_ = -1, locTex_ = -1, locM_ = -1, locRot_ = -1;
    GLint locSat_ = -1, locCon_ = -1, locExp_ = -1, locTemp_ = -1, locStr_ = -1;
};

}  // namespace camengine

#endif
