#version 330 core
in float vAlpha;
in vec2  vUV;

uniform vec3 trailColor;

out vec4 FragColor;

void main()
{
    // 沿刀刃寬度方向做邊緣淡化（v=0或1時更透明）
    float edgeFade = smoothstep(0.0, 0.15, vUV.y) *
                     smoothstep(1.0, 0.85, vUV.y);

    float alpha = vAlpha * edgeFade;

    FragColor = vec4(trailColor, alpha);
}