#version 330 core
out vec4 FragColor;

in vec4 clipSpacePos;
in vec3 worldPos;
in vec3 toCamera;

uniform sampler2D reflectionTexture;
uniform float     time;
uniform vec3      lightDir;
uniform vec3      lightColor;

void main()
{
    // Projective UV for reflection texture (Y-flipped to show mirror image)
    vec2 ndc       = clipSpacePos.xy / clipSpacePos.w;
    vec2 reflectUV = vec2(-ndc.x, ndc.y) * 0.5 + 0.5;

    // Procedural wave distortion (replaces DuDv map)
    float waveX = sin(worldPos.x * 0.8 + time * 1.5) * 0.012;
    float waveZ = sin(worldPos.z * 0.6 + time * 1.2) * 0.012;

    reflectUV = clamp(reflectUV + vec2(waveX, waveZ), 0.001, 0.999);
    vec4 reflectionColor = texture(reflectionTexture, reflectUV);

    // Procedural water normal for Fresnel and specular
    float dnX       = cos(worldPos.x * 0.8 + time * 1.5) * 0.15;
    float dnZ       = cos(worldPos.z * 0.6 + time * 1.2) * 0.15;
    vec3  waterNorm = normalize(vec3(-dnX, 1.0, -dnZ));

    // Fresnel: grazing angle = more reflection, top-down = more transparency
    vec3  camDir = normalize(toCamera);
    float fresnel = pow(1.0 - max(dot(camDir, waterNorm), 0.0), 3.0);
    fresnel = clamp(fresnel * 0.9 + 0.1, 0.0, 1.0);

    vec4 waterColor = vec4(0.0, 0.15, 0.35, 0.85);
    vec4 blended    = mix(waterColor, reflectionColor, fresnel);

    // Blinn-Phong specular highlight
    vec3  halfV = normalize(normalize(-lightDir) + camDir);
    float spec  = pow(max(dot(waterNorm, halfV), 0.0), 256.0);
    blended.rgb += lightColor * spec * 0.6;

    FragColor = blended;
}
