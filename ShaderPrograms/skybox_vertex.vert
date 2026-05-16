#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    // 移除 view 矩陣的位移分量，確保天空盒隨攝影機旋轉但不隨位移移動
    vec4 pos = projection * mat4(mat3(view)) * vec4(aPos, 1.0);
    // 將 z 設為 w，使深度測試後 depth = 1.0（最遠處），不會遮擋任何場景物件
    gl_Position = pos.xyww;
}
