#version 330 core
// 顏色拾取 Pass 的頂點著色器
// VAO 佈局與 worldObject 著色器相同（location 0~2 + instanced mat4 @ 7）
// 僅輸出裁切空間座標，不需要法線或 UV
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 7) in mat4 InstanceMatrix;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * InstanceMatrix * vec4(aPos, 1.0);
}
