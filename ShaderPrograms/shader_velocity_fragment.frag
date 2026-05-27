#version 460 core

// ── 輸入 ─────────────────────────────────────────────────────────────────────
in vec4 v_currClip;   // 當前幀 Clip-space 位置
in vec4 v_prevClip;   // 前一幀 Clip-space 位置

// ── 輸出：RG16F 速度向量（UV 空間偏移量）─────────────────────────────────────
layout(location = 0) out vec2 fragVelocity;

void main()
{
    // Perspective divide → NDC 座標 ([-1, 1])
    vec2 currNDC = v_currClip.xy / v_currClip.w;
    vec2 prevNDC = v_prevClip.xy / v_prevClip.w;

    // 速度 = NDC 空間位移的一半 = UV 空間位移（UV 範圍 [0, 1]）
    // 正值代表向右/上移動，負值代表向左/下移動
    fragVelocity = (currNDC - prevNDC) * 0.5;
}
