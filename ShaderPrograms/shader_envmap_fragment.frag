#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform samplerCube skybox;     // bound to texture unit 1
uniform vec3  cameraPos;
uniform int   envMode;          // 1=reflect, 2=refract
uniform float refractRatio;     // e.g. 0.658 for glass

void main()
{
    vec3 I = normalize(FragPos - cameraPos);
    vec3 N = normalize(Normal);
    vec3 sampleDir;

    if (envMode == 1)
    {
        sampleDir = reflect(I, N);
    }
    else
    {
        sampleDir = refract(I, N, refractRatio);
        if (length(sampleDir) < 0.001)
            sampleDir = reflect(I, N);  // total internal reflection fallback
    }

    FragColor = texture(skybox, sampleDir);
}
