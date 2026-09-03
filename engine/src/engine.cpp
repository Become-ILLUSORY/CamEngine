/*
 * 引擎实现（P1）
 * 实现曲线采样、LUT 烘焙、滤镜生命周期。
 * 渲染仍由 GPU 侧调用 engine_render 传入的已烘焙 LUT 完成。
 */
#include "engine_api.h"
#include "engine_internal.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

namespace camengine {

namespace {

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/* 保单调三次 Hermite 采样：
 * 把控制点表建成 kCurveLutSize 项，使用 Fritsch-Carlson 保单调切线。
 * 曲线输入/输出都归一化到 [0,1]。
 */
void curveBuildTable(Curve& c) {
    const int n = c.count;
    for (int i = 0; i < kCurveLutSize; ++i) {
        if (n == 0) {
            c.table[i] = i / (float)(kCurveLutSize - 1);
        } else if (n == 1) {
            c.table[i] = c.ys[0];
        } else {
            // 默认恒等线 (0,0)->(1,1)，若用户只给一个点，兜底为恒等
            float x = i / (float)(kCurveLutSize - 1);
            c.table[i] = x;
        }
    }
    if (n < 2) { c.dirty = false; return; }

    // 确保 x 单调递增
    // 对每段 [xs[k], xs[k+1]] 做 Hermite
    float xd[kMaxCurvePoints], yd[kMaxCurvePoints];
    float dx[kMaxCurvePoints], dy[kMaxCurvePoints], slope[kMaxCurvePoints];
    float m[kMaxCurvePoints];

    // 用末点确保端点
    // 此处 xs,ys 视为 0..1
    // 计算分段斜率
    for (int k = 0; k < n - 1; ++k) {
        dx[k] = c.xs[k + 1] - c.xs[k];
        dy[k] = c.ys[k + 1] - c.ys[k];
        if (dx[k] <= 0) dx[k] = 1e-4f;
        slope[k] = dy[k] / dx[k];
    }

    // Fritsch-Carlson 端点切线
    m[0] = (n >= 2) ? slope[0] : 0.f;
    m[n - 1] = (n >= 2) ? slope[n - 2] : 0.f;
    for (int k = 1; k < n - 1; ++k) {
        if (slope[k - 1] * slope[k] < 0) {
            m[k] = 0.f;  // 单调翻转点切向 0，保单调
        } else {
            float w1 = 2.f * dx[k] + dx[k - 1];
            float w2 = dx[k] + 2.f * dx[k - 1];
            m[k] = (w1 + w2) > 0 ? (w1 + w2) / (w1 / slope[k - 1] + w2 / slope[k]) : 0.f;
        }
    }

    // 采样
    for (int i = 0; i < kCurveLutSize; ++i) {
        float t = i / (float)(kCurveLutSize - 1);
        // 二分/线性找段
        int k = 0;
        for (int j = 0; j < n - 1; ++j) {
            if (t >= c.xs[j]) k = j;
        }
        float x0 = c.xs[k], x1 = c.xs[k + 1];
        float y0 = c.ys[k], y1 = c.ys[k + 1];
        float h = x1 - x0; if (h <= 0) h = 1e-4f;
        float s = (t - x0) / h;
        s = clampf(s, 0.f, 1.f);
        float h00 = 2*s*s*s - 3*s*s + 1;
        float h10 = s*s*s - 2*s*s + s;
        float h01 = -2*s*s*s + 3*s*s;
        float h11 = s*s*s - s*s;
        float val = h00*y0 + h10*(h*m[k]) + h01*y1 + h11*(h*m[k+1]);
        c.table[i] = clampf(val, 0.f, 1.f);
    }
    c.dirty = false;
}

float curveEval(const Curve& c, float x01) {
    float x = clampf(x01, 0.f, 1.f);
    float fi = x * (kCurveLutSize - 1);
    int i0 = (int)fi;
    if (i0 >= kCurveLutSize - 1) return c.table[kCurveLutSize - 1];
    float frac = fi - i0;
    return c.table[i0] * (1.f - frac) + c.table[i0 + 1] * frac;
}

}  // namespace

}  // namespace camengine

using namespace camengine;

/* ========== 生命周期 ========== */
void* engine_create(void* gpu_context) {
    Engine* e = new Engine();
    e->gpu_context = gpu_context;
    e->inited = (gpu_context != nullptr) ? 1 : 0;
    return e;
}

void engine_destroy(void* engine) {
    if (engine) delete (Engine*)engine;
}

/* ========== 版本 & 能力 ========== */
EngineVersion engine_get_version(void) {
    EngineVersion v;
    v.major = 0; v.minor = 2; v.patch = 0;
    v.build = "p1-curve-lut";
    return v;
}

void engine_get_device_caps(void* engine, DeviceCaps* caps) {
    (void)engine;
    if (!caps) return;
    caps->max_lut_size = kLutSize;
    caps->max_texture_size = 8192;
    caps->supports_gles3 = 1;
    caps->supports_vulkan = 0;
    caps->gpu_name = "unknown";
}

/* ========== 滤镜生命周期 ========== */
FilterHandle engine_filter_create(void* engine) {
    Engine* e = (Engine*)engine;
    if (!e) return nullptr;
    Filter* f = new Filter();
    e->filters.push_back(f);
    return (FilterHandle)f;
}

void engine_filter_destroy(void* engine, FilterHandle filter) {
    Engine* e = (Engine*)engine;
    Filter* f = (Filter*)filter;
    if (!e || !f) return;
    for (auto it = e->filters.begin(); it != e->filters.end(); ++it) {
        if (*it == f) { delete f; e->filters.erase(it); return; }
    }
}

/* ========== 曲线编辑 ========== */
void engine_curve_set_points(void* engine, FilterHandle filter, int channel,
                             const float* xs, const float* ys, int count) {
    (void)engine;
    Filter* f = (Filter*)filter;
    if (!f || channel < 0 || channel >= kChannelCount || !xs || !ys) return;
    if (count < 0 || count > kMaxCurvePoints) return;
    Curve& c = f->curves[channel];
    c.count = count;
    for (int i = 0; i < count; ++i) {
        c.xs[i] = clampf(xs[i], 0.f, 1.f);
        c.ys[i] = clampf(ys[i], 0.f, 1.f);
    }
    c.dirty = true;
    f->lutDirty = true;
}

void engine_curve_move_point(void* engine, FilterHandle filter, int channel,
                             int index, float x, float y) {
    (void)engine;
    Filter* f = (Filter*)filter;
    if (!f || channel < 0 || channel >= kChannelCount) return;
    Curve& c = f->curves[channel];
    if (index < 0 || index >= c.count) return;
    c.xs[index] = clampf(x, 0.f, 1.f);
    c.ys[index] = clampf(y, 0.f, 1.f);
    c.dirty = true;
    f->lutDirty = true;
}

void engine_curve_add_point(void* engine, FilterHandle filter, int channel,
                            float x, float y) {
    (void)engine;
    Filter* f = (Filter*)filter;
    if (!f || channel < 0 || channel >= kChannelCount) return;
    Curve& c = f->curves[channel];
    if (c.count >= kMaxCurvePoints) return;
    int idx = c.count;
    for (int i = 0; i < c.count; ++i) if (x < c.xs[i]) { idx = i; break; }
    for (int i = c.count; i > idx; --i) { c.xs[i] = c.xs[i-1]; c.ys[i] = c.ys[i-1]; }
    c.xs[idx] = clampf(x, 0.f, 1.f);
    c.ys[idx] = clampf(y, 0.f, 1.f);
    c.count++;
    c.dirty = true;
    f->lutDirty = true;
}

void engine_curve_remove_point(void* engine, FilterHandle filter, int channel,
                               int index) {
    (void)engine;
    Filter* f = (Filter*)filter;
    if (!f || channel < 0 || channel >= kChannelCount) return;
    Curve& c = f->curves[channel];
    if (c.count <= 2) return;  // 至少保留两端
    if (index < 0 || index >= c.count) return;
    for (int i = index; i < c.count - 1; ++i) { c.xs[i] = c.xs[i+1]; c.ys[i] = c.ys[i+1]; }
    c.count--;
    c.dirty = true;
    f->lutDirty = true;
}

/* ========== 通用参数 ========== */
void engine_filter_set_param(void* engine, FilterHandle filter,
                             const char* name, float value) {
    (void)engine;
    Filter* f = (Filter*)filter;
    if (!f || !name) return;
    struct { const char* n; Param p; } map[] = {
        {"strength", kParamStrength}, {"exposure", kParamExposure},
        {"contrast", kParamContrast}, {"saturation", kParamSaturation},
        {"temperature", kParamTemperature}, {"tint", kParamTint},
        {"vignette", kParamVignette}, {"grain", kParamGrain},
    };
    for (auto& m : map) {
        if (strcmp(m.n, name) == 0) {
            f->params[m.p] = value;
            f->lutDirty = true;
            return;
        }
    }
}

/* ========== LUT 烘焙 ==========
 * 把当前滤镜状态（各通道曲线 + 参数）烘焙成 3D LUT。
 * LUT 存于 filter->lut，返回纹理 ID（GPU 端由上层据此上传纹理，此处返回哨兵 0）。
 */
uint32_t engine_bake_lut(void* engine, FilterHandle filter) {
    (void)engine;
    Filter* f = (Filter*)filter;
    if (!f) return 0;
    // 确保各曲线表是最新
    for (int ch = 0; ch < kChannelCount; ++ch) {
        if (f->curves[ch].dirty) curveBuildTable(f->curves[ch]);
    }

    const int n = kLutSize; /* 33 */
    const float temp = f->params[kParamTemperature] / 100.f; /* -1..1 */
    const float sat = 1.f + f->params[kParamSaturation] / 100.f;
    const float expo = f->params[kParamExposure]; /* -1..1 stops 简化 */
    const float ctr = f->params[kParamContrast] / 100.f; /* -1..1 */

    for (int ir = 0; ir < n; ++ir) {
        for (int ig = 0; ig < n; ++ig) {
            for (int ib = 0; ib < n; ++ib) {
                float r = ir / (float)(n - 1);
                float g = ig / (float)(n - 1);
                float b = ib / (float)(n - 1);

                // 1) 通用参数
                float luma = 0.2126f*r + 0.7152f*g + 0.0722f*b;
                // 曝光：lift 到 log 域
                r *= exp2f(expo); g *= exp2f(expo); b *= exp2f(expo);
                // 对比度（绕灰点 0.5）
                r = 0.5f + (r - 0.5f) * (1.f + ctr);
                g = 0.5f + (g - 0.5f) * (1.f + ctr);
                b = 0.5f + (b - 0.5f) * (1.f + ctr);
                // 饱和度
                r = luma + (r - luma) * sat;
                g = luma + (g - luma) * sat;
                b = luma + (b - luma) * sat;
                // 色温（近似：暖 = 加 R 减 B）
                if (temp > 0) { r += temp*0.2f; b -= temp*0.2f; }
                else { r += temp*0.2f; b -= temp*0.2f; }
                r = clampf(r, 0.f, 1.f); g = clampf(g, 0.f, 1.f); b = clampf(b, 0.f, 1.f);

                // 2) 各通道曲线（先 RGB 联合，再独立通道，后 Luma）
                float rr = curveEval(f->curves[kChannelRGB], r);
                float gg = curveEval(f->curves[kChannelRGB], g);
                float bb = curveEval(f->curves[kChannelRGB], b);
                rr = curveEval(f->curves[kChannelRed], rr);
                gg = curveEval(f->curves[kChannelGreen], gg);
                bb = curveEval(f->curves[kChannelBlue], bb);
                // Luma 曲线：作用于亮度分量，用缩放保持色相
                float luma2 = 0.2126f*rr + 0.7152f*gg + 0.0722f*bb;
                float scale = (luma2 > 1e-4f) ? curveEval(f->curves[kChannelLuma], luma2) / luma2 : 1.f;
                rr = clampf(rr * scale, 0.f, 1.f);
                gg = clampf(gg * scale, 0.f, 1.f);
                bb = clampf(bb * scale, 0.f, 1.f);

                f->lut[ir][ig][ib][0] = rr;
                f->lut[ir][ig][ib][1] = gg;
                f->lut[ir][ig][ib][2] = bb;
            }
        }
    }
    f->lutDirty = false;
    return 0; /* 上层据此生成纹理 */
}

/* ========== 渲染 ==========
 * GPU 端已持有 f->lut 生成的纹理，此处占位。
 * 真正的片段着色器查表在 GPU 侧（P1.5 GLES 管线）。
 */
void engine_render(void* engine, uint32_t in_tex, uint32_t out_tex,
                   FilterHandle filter, float strength) {
    (void)engine; (void)in_tex; (void)out_tex; (void)filter; (void)strength;
    // 若 LUT 未烘焙则先烘焙
    Filter* f = (Filter*)filter;
    if (f && f->lutDirty) engine_bake_lut(engine, filter);
}

/* ========== 导出 ========== */
char* engine_filter_export_json(void* engine, FilterHandle filter) {
    (void)engine;
    Filter* f = (Filter*)filter;
    if (!f) return nullptr;
    // 简单导出曲线点（完整 JSON 格式见 ARCHITECTURE.md，后续实现）
    size_t cap = 1024;
    char* out = (char*)malloc(cap);
    if (!out) return nullptr;
    int off = 0;
    off += snprintf(out + off, cap - off, "{\"schemaVersion\":1");
    for (int ch = 0; ch < kChannelCount; ++ch) {
        Curve& c = f->curves[ch];
        off += snprintf(out + off, cap - off, ",\"ch%d\":[", ch);
        for (int i = 0; i < c.count; ++i)
            off += snprintf(out + off, cap - off, "%s[%.4f,%.4f]", i ? "," : "", c.xs[i], c.ys[i]);
        off += snprintf(out + off, cap - off, "]");
    }
    off += snprintf(out + off, cap - off, "}");
    return out;
}

/* ========== JSON 导入（精简实现，完整校验后续补） ========== */
int engine_filter_load_json(void* engine, FilterHandle filter, const char* json) {
    (void)engine;
    Filter* f = (Filter*)filter;
    if (!f || !json) return -1;
    // TODO: 完整 JSON 解析（P2）
    return -1;
}
