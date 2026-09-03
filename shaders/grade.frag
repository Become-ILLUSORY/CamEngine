#include <flutter/runtime_effect.glsl>

// M1 实时调色验证着色器
// 输入: 一张图 + 实时滑杆参数
// 输出: GPU 实时调色结果（所见即所得）
uniform vec2 uSize;
uniform shader uImage;
uniform float uSaturation;   // 0.0..2.0, 1.0=中性
uniform float uContrast;     // 0.0..2.0, 1.0=中性
uniform float uExposure;     // 0.25..2.0, 1.0=原始
uniform float uTemperature;  // -1.0..1.0, 正=暖(加R减B)
uniform float uStrength;     // 0.0..1.0 滤镜强度

out vec4 fragColor;

void main() {
    vec2 uv = FlutterFragCoord().xy / uSize.xy;
    vec4 c = uImage.eval(uv);
    vec3 col = c.rgb;

    // 亮度
    float luma = dot(col, vec3(0.2126, 0.7152, 0.0722));

    // 饱和度（绕灰点）
    col = mix(vec3(luma), col, uSaturation);

    // 对比度（绕 0.5）
    col = (col - 0.5) * uContrast + 0.5;

    // 曝光（线性乘）
    col *= uExposure;

    // 色温（近似：暖=加R减B，冷=减R加B）
    col.r += uTemperature * 0.2;
    col.b -= uTemperature * 0.2;

    // 强度混合：0=原图，1=完全滤镜
    col = mix(c.rgb, col, uStrength);

    fragColor = vec4(clamp(col, 0.0, 1.0), c.a);
}
