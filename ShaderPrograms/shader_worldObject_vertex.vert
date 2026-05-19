#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 7) in mat4 InstanceMatrix;

out vec3 vVaryingNormal;
out vec3 vVaryingLightDir;
out vec2 TexCoords;
out vec4 vFragPosLightSpace;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform vec4 clipPlane;

uniform int  lightType;       // 0 = Directional, 1 = Point
uniform vec3 lightPosition;   // 世界座標（Point Light 用）
uniform vec3 lightDirection;  // 世界座標方向（Directional Light 用）

void main()
{
    mat4 MVP = projection * view * InstanceMatrix;
    mat4 MV  = view * InstanceMatrix;

    mat3 normalMatrix = transpose(inverse(mat3(MV)));
    vVaryingNormal = normalMatrix * aNormal;

    vec4 vPosition4 = MV * vec4(aPos, 1.0);
    vec3 vPosition3 = vPosition4.xyz / vPosition4.w;

    if (lightType == 0)
    {
        // Directional: 將世界空間方向轉至 eye space，取反後得到指向光源的向量
        vec3 dirEye = normalize(mat3(view) * (-normalize(lightDirection)));
        vVaryingLightDir = dirEye;
    }
    else
    {
        // Point: 從 eye-space 頂點位置指向 eye-space 光源位置
        vec3 lightPosEye = vec3(view * vec4(lightPosition, 1.0));
        vVaryingLightDir = normalize(lightPosEye - vPosition3);
    }

    TexCoords = aTexCoords;
    vFragPosLightSpace = lightSpaceMatrix * InstanceMatrix * vec4(aPos, 1.0);
    gl_ClipDistance[0] = dot(InstanceMatrix * vec4(aPos, 1.0), clipPlane);
    gl_Position = MVP * vec4(aPos, 1.0);
}
