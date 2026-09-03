/*
 * CamEngine 内部实现头文件
 * 仅供引擎源码使用，不对外导出。
 */
#ifndef CAMENGINE_ENGINE_INTERNAL_H_
#define CAMENGINE_ENGINE_INTERNAL_H_

#include <stdint.h>
#include <vector>

namespace camengine {

/* 最大控制点数（曲线编辑器使用） */
constexpr int kMaxCurvePoints = 32;
/* 曲线离散表分辨率 */
constexpr int kCurveLutSize = 256;
/* 3D LUT 默认边长（烘焙后渲染查表用） */
constexpr int kLutSize = 33;

/* 通道枚举，与 engine_api.h 注释一致 */
enum Channel {
    kChannelRGB = 0,
    kChannelRed = 1,
    kChannelGreen = 2,
    kChannelBlue = 3,
    kChannelLuma = 4,
    kChannelCount = 5,
};

/* 曲线参数（通用调色） */
enum Param {
    kParamStrength = 0,
    kParamExposure,
    kParamContrast,
    kParamSaturation,
    kParamTemperature,
    kParamTint,
    kParamVignette,
    kParamGrain,
    kParamCount,
};

/* 一条可调曲线：控制点 + 离散表 */
struct Curve {
    float xs[kMaxCurvePoints];
    float ys[kMaxCurvePoints];
    int count = 0;
    bool dirty = true;
    float table[kCurveLutSize]; /* 采样后的单调曲线，0..255 归一化表 */

    void clear() { count = 0; dirty = true; }

    /* 采样控制点 -> table[]，采用保单调三次 Hermite */
    void buildTable();
    /* 单点查询（归一化 0..1 输入返回 0..1） */
    float eval(float x01) const;
};

/* 滤镜对象（对应一个社区滤镜/曲线组） */
struct Filter {
    Curve curves[kChannelCount];
    float params[kParamCount] = {1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 0.0f};
    bool lutDirty = true;
    float lut[kLutSize][kLutSize][kLutSize][3]; /* RGB 输出表 */
};

/* 引擎主对象 */
struct Engine {
    void* gpu_context = nullptr;
    int inited = 0;
    std::vector<Filter*> filters;

    ~Engine() { for (auto f : filters) delete f; }
};

}  // namespace camengine

#endif /* CAMENGINE_ENGINE_INTERNAL_H_ */
