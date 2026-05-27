#version 460 core

// ── 頂點屬性 ─────────────────────────────────────────────────────────────────
layout(location = 0) in vec3 aPos;
// location 7–10：InstanceMatrix（mat4，由 instanceVBO 傳入，每個 instance 不同）
layout(location = 7) in mat4 InstanceMatrix;   // 當前幀的 Model Matrix

// ── Uniform ───────────────────────────────────────────────────────────────────
uniform mat4 view;          // 當前幀 View Matrix
uniform mat4 projection;    // 當前幀 Projection Matrix
uniform mat4 u_prevModel;   // 前一幀的 Model Matrix（同一物件，由 C++ 每物件設定）
uniform mat4 u_prevVP;      // 前一幀的 View * Projection（整場景相同）

// ── 輸出 ─────────────────────────────────────────────────────────────────────
out vec4 v_currClip;        // 當前幀的 Clip-space 位置
out vec4 v_prevClip;        // 前一幀的 Clip-space 位置

void main()
{
    mat4 currVP = projection * view;

    // 當前幀：使用 instanceVBO 的 InstanceMatrix 作為 Model Matrix
    v_currClip  = currVP  * InstanceMatrix * vec4(aPos, 1.0);

    // 前一幀：使用 uniform 傳入的上一幀 Model Matrix
    v_prevClip  = u_prevVP * u_prevModel   * vec4(aPos, 1.0);

    gl_Position = v_currClip;
}
