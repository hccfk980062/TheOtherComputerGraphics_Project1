#version 460 core

// ── 輸入 ─────────────────────────────────────────────────────────────────────
in vec2 TexCoords;

// ── 輸出 ─────────────────────────────────────────────────────────────────────
out vec4 FragColor;

// ── Uniform ───────────────────────────────────────────────────────────────────
uniform sampler2D u_screenTexture;    // 場景顏色貼圖（viewportFBO）
uniform sampler2D u_velocityTexture;  // 速度向量貼圖（velocityFBO，RG16F）
uniform int       u_numSamples;       // 採樣次數（2~16）
uniform float     u_blurStrength;     // 速度縮放係數（提高此值加強模糊效果）

void main()
{
    // 讀取此像素的螢幕空間速度向量並套用強度縮放
    vec2 velocity = texture(u_velocityTexture, TexCoords).rg * u_blurStrength;

    // ── 限制最大模糊距離（防止快速移動或縮放時失真）────────────────────────────
    float speed      = length(velocity);
    const float maxBlur = 0.05;  // 最多模糊畫面寬度的 5%
    if (speed > maxBlur)
        velocity = normalize(velocity) * maxBlur;

    // ── 速度過小時直接輸出原色（避免不必要的迴圈開銷）─────────────────────────
    if (speed < 0.0001)
    {
        FragColor = texture(u_screenTexture, TexCoords);
        return;
    }

    // ── 沿速度向量反方向均勻取樣並平均（模擬快門曝光積分）──────────────────────
    // t=0：當前位置；t=1：上一幀位置（velocity 指向當前幀的方向，所以反向取樣）
    int   samples  = clamp(u_numSamples, 2, 16);
    vec4  color    = vec4(0.0);
    float fSamples = float(samples);

    for (int i = 0; i < samples; i++)
    {
        float t       = float(i) / (fSamples - 1.0);
        vec2  sampleUV = TexCoords - velocity * t;
        sampleUV       = clamp(sampleUV, 0.001, 0.999);  // 防止邊界溢出造成拉伸
        color         += texture(u_screenTexture, sampleUV);
    }

    FragColor = color / fSamples;
}
