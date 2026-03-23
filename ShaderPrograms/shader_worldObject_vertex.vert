#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

vec3 vLightPosition = vec3(0, 10, 50);

out vec3 vVaryingNormal;
out vec3 vVaryingLightDir;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 MVP = projection * view * model;
    mat4 MV  = view * model;

    // ── 修正：Normal Matrix 必須用 transpose(inverse(...)) ────────────
    // 僅取 mat3(MV) 在模型有非均勻縮放時會導致法向量扭曲，光照計算錯誤
    mat3 normalMatrix = transpose(inverse(mat3(MV)));   // ← 修正
    vVaryingNormal = normalMatrix * aNormal;

    // Get vertex position in eye coordinates
    vec4 vPosition4 = MV * vec4(aPos, 1.0);
    vec3 vPosition3 = vPosition4.xyz / vPosition4.w;

    // Get vector to light source
    vVaryingLightDir = normalize(vLightPosition - vPosition3);

    TexCoords  = aTexCoords;
    gl_Position = MVP * vec4(aPos, 1.0);
}
