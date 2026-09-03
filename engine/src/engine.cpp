/*
 * 引擎实现（P0 骨架）
 * 当前阶段仅实现：生命周期、版本、能力查询。
 * LUT/曲线/渲染将在 P1 填充。
 */
#include "engine_api.h"

#include <stdlib.h>
#include <string.h>

namespace camengine {

struct Engine {
    void* gpu_context;
    int inited;
};

}  // namespace camengine

using camengine::Engine;

/* ---------- 生命周期 ---------- */
void* engine_create(void* gpu_context) {
    Engine* e = (Engine*)calloc(1, sizeof(Engine));
    if (!e) return nullptr;
    e->gpu_context = gpu_context;
    e->inited = (gpu_context != nullptr) ? 1 : 0;
    return e;
}

void engine_destroy(void* engine) {
    free(engine);
}

/* ---------- 版本 & 能力 ---------- */
EngineVersion engine_get_version(void) {
    EngineVersion v;
    v.major = 0;
    v.minor = 1;
    v.patch = 0;
    v.build = "p0-skeleton";
    return v;
}

void engine_get_device_caps(void* engine, DeviceCaps* caps) {
    (void)engine;
    if (!caps) return;
    caps->max_lut_size = 64;
    caps->max_texture_size = 8192;
    caps->supports_gles3 = 1;
    caps->supports_vulkan = 0;
    caps->gpu_name = "unknown";
}

/* ---------- 滤镜生命周期（P1 实现） ---------- */
FilterHandle engine_filter_create(void* engine) {
    (void)engine;
    return nullptr;
}
void engine_filter_destroy(void* engine, FilterHandle filter) {
    (void)engine; (void)filter;
}
int engine_filter_load_json(void* engine, FilterHandle filter, const char* json) {
    (void)engine; (void)filter; (void)json;
    return -1; /* 未实现 */
}

/* ---------- 曲线编辑（P1 实现） ---------- */
void engine_curve_set_points(void* engine, FilterHandle filter, int channel,
                             const float* xs, const float* ys, int count) {
    (void)engine; (void)filter; (void)channel; (void)xs; (void)ys; (void)count;
}
void engine_curve_move_point(void* engine, FilterHandle filter, int channel,
                             int index, float x, float y) {
    (void)engine; (void)filter; (void)channel; (void)index; (void)x; (void)y;
}
void engine_curve_add_point(void* engine, FilterHandle filter, int channel,
                            float x, float y) {
    (void)engine; (void)filter; (void)channel; (void)x; (void)y;
}
void engine_curve_remove_point(void* engine, FilterHandle filter, int channel,
                               int index) {
    (void)engine; (void)filter; (void)channel; (void)index;
}

/* ---------- 通用参数（P1 实现） ---------- */
void engine_filter_set_param(void* engine, FilterHandle filter,
                             const char* name, float value) {
    (void)engine; (void)filter; (void)name; (void)value;
}

/* ---------- 烘焙 & 渲染（P1 实现） ---------- */
uint32_t engine_bake_lut(void* engine, FilterHandle filter) {
    (void)engine; (void)filter;
    return 0;
}
void engine_render(void* engine, uint32_t in_tex, uint32_t out_tex,
                   FilterHandle filter, float strength) {
    (void)engine; (void)in_tex; (void)out_tex; (void)filter; (void)strength;
}

/* ---------- 导出（P1 实现） ---------- */
char* engine_filter_export_json(void* engine, FilterHandle filter) {
    (void)engine; (void)filter;
    char* out = (char*)malloc(2);
    if (out) { out[0] = '\0'; }
    return out;
}
