#version 330 core
out vec4 FragColor;

in vec3 vVaryingNormal;
in vec3 vVaryingLightDir;
in vec2 TexCoords;
in vec4 vFragPosLightSpace;

struct Material
{
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    vec3  emissive;
    float emissiveStrength;
    float shininess;
};

uniform bool      useTextures;
uniform Material  material;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_emissive1;
uniform sampler2D shadowMap;

uniform vec3  lightColor;
uniform float lightIntensity;
uniform float shadowBias;

// 3×3 PCF Shadow Sampling
float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // 超出 shadow map 範圍或超過遠裁切面→視為無陰影
    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - shadowBias) > pcfDepth ? 1.0 : 0.0;
        }
    return shadow / 9.0;
}

void main()
{
    // ── 取得 Diffuse 顏色 ─────────────────────────────────────────────────
    vec4 diffuseColor = useTextures
        ? texture(texture_diffuse1, TexCoords)
        : vec4(material.diffuse, 1.0);

    // ── 自發光覆蓋（有自發光時直接輸出，跳過光照計算）────────────────────
    vec3 emission = material.emissive * material.emissiveStrength;
    if (emission != vec3(0.0))
    {
        FragColor = vec4(emission, 1.0);
        return;
    }

    // ── Phong Lighting ───────────────────────────────────────────────────
    vec3  lightEff = lightColor * lightIntensity;
    vec3  norm     = normalize(vVaryingNormal);
    vec3  lightDir = normalize(vVaryingLightDir);
    float diff     = max(0.0, dot(norm, lightDir));

    // Specular（Blinn-Phong 反射向量）
    vec3  vReflection = normalize(reflect(-lightDir, norm));
    float shininess   = material.shininess > 0.0 ? material.shininess : 32.0;
    float spec        = (diff > 0.0) ? pow(max(0.0, dot(norm, vReflection)), shininess) : 0.0;

    vec3 specSample = useTextures
        ? texture(texture_specular1, TexCoords).rgb
        : material.specular;

    // ── 陰影係數（PCF）───────────────────────────────────────────────────
    float shadow = ShadowCalculation(vFragPosLightSpace);

    // ── 合成：Ambient + (1 - shadow) * (Diffuse + Specular) ──────────────
    FragColor = vec4(0.1 * lightEff, 1.0) * diffuseColor;
    FragColor += (1.0 - shadow) * (diff * vec4(lightEff, 1.0) * diffuseColor
                                 + vec4(specSample * lightEff * spec, 1.0));
}
