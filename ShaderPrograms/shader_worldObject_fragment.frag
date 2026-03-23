#version 330 core
out vec4 FragColor;

in vec3 vVaryingNormal;
in vec3 vVaryingLightDir;
in vec2 TexCoords;

struct Material 
{
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};

uniform bool      useTextures;

uniform Material      material;
uniform sampler2D     texture_diffuse1;
uniform sampler2D     texture_specular1;
uniform sampler2D     texture_normal1;

vec4    ambientColor  = vec4(0.1, 0.1, 0.1, 1);
vec4    specularColor = vec4(1, 1, 1, 1);
float   Shininess     = 128.0;

void main()
{
    // ── 取得 diffuse 顏色（修正：useTextures 時從貼圖取樣）──────────
    vec4 diffuseColor;
    if (useTextures)
        diffuseColor = texture(texture_diffuse1, TexCoords);   // ← 修正：實際取樣貼圖
    else
        diffuseColor = vec4(material.diffuse, 1.0);            // 無貼圖時用材質顏色

    // ── Diffuse lighting ─────────────────────────────────────────────
    float diff = max(0.0, dot(normalize(vVaryingNormal),
                              normalize(vVaryingLightDir)));

    FragColor = diff * diffuseColor;

    // ── Ambient ──────────────────────────────────────────────────────
    FragColor += ambientColor * diffuseColor;

    // ── Specular ─────────────────────────────────────────────────────
    vec3 vReflection = normalize(reflect(-normalize(vVaryingLightDir),
                                          normalize(vVaryingNormal)));
    float spec = max(0.0, dot(normalize(vVaryingNormal), vReflection));
    if (diff != 0.0) {
        spec = pow(spec, Shininess);

        // 有貼圖時從 specular map 取樣，否則用材質 specular 屬性
        vec3 specSample = useTextures
            ? texture(texture_specular1, TexCoords).rgb
            : material.specular;

        FragColor += vec4(specSample, 1.0) * specularColor * spec;
    }
}
