/*
 * CamEngine 统一 C API
 * 这是 C++ 引擎对外的唯一接口，三端（Android JNI / iOS / 鸿蒙 NAPI）都调用它。
 * 保证引擎核心与平台解耦。
 */
#ifndef CAMENGINE_ENGINE_API_H_
#define CAMENGINE_ENGINE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 引擎版本信息 */
typedef struct EngineVersion {
    int major;
    int minor;
    int patch;
    const char* build;   /* 构建标识 */
} EngineVersion;

/* 设备能力描述（GPU/纹理/LUT 尺寸上限等） */
typedef struct DeviceCaps {
    int max_lut_size;          /* 支持的 LUT 边长上限 */
    int max_texture_size;      /* 最大纹理尺寸 */
    int supports_gles3;        /* 是否支持 GLES 3 */
    int supports_vulkan;       /* 是否支持 Vulkan */
    const char* gpu_name;      /* GPU 名称（调试用） */
} DeviceCaps;

/* 滤镜句柄 */
typedef void* FilterHandle;

/*
 * 生命周期
 * gpu_context: 平台传入的 GPU 上下文（Android 传 EGLContext，iOS 传 MTLCtx 等）。
 *              引擎内部据此创建自己的渲染资源。
 */
void*   engine_create(void* gpu_context);
void    engine_destroy(void* engine);

/* 版本 & 能力查询 */
EngineVersion engine_get_version(void);
void          engine_get_device_caps(void* engine, DeviceCaps* caps);

/* 滤镜生命周期 */
FilterHandle engine_filter_create(void* engine);
void         engine_filter_destroy(void* engine, FilterHandle filter);
int          engine_filter_load_json(void* engine, FilterHandle filter,
                                     const char* json);

/* 曲线编辑 */
/* channel: 0=RGB, 1=Red, 2=Green, 3=Blue, 4=Luma */
void engine_curve_set_points(void* engine, FilterHandle filter, int channel,
                             const float* xs, const float* ys, int count);
void engine_curve_move_point(void* engine, FilterHandle filter, int channel,
                             int index, float x, float y);
void engine_curve_add_point(void* engine, FilterHandle filter, int channel,
                            float x, float y);
void engine_curve_remove_point(void* engine, FilterHandle filter, int channel,
                               int index);

/* 通用参数控制 */
/* name 取值: "strength","exposure","contrast","saturation","temperature",
 *           "tint","vignette","grain","shadows","highlights","whites","blacks" */
void engine_filter_set_param(void* engine, FilterHandle filter,
                             const char* name, float value);

/* 烘焙 LUT：把当前滤镜状态烘焙为 LUT 纹理，返回纹理 ID */
uint32_t engine_bake_lut(void* engine, FilterHandle filter);

/*
 * 渲染一帧
 * input_texture_id / output_texture_id: 平台纹理 ID（GLES 为 GLuint）
 * strength: 滤镜强度 0.0~1.0
 */
void engine_render(void* engine, uint32_t input_texture_id,
                   uint32_t output_texture_id, FilterHandle filter,
                   float strength);

/* 导出滤镜 JSON（调用者 free 返回的字符串） */
char* engine_filter_export_json(void* engine, FilterHandle filter);

#ifdef __cplusplus
}
#endif

#endif /* CAMENGINE_ENGINE_API_H_ */
