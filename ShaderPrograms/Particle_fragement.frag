#version 330 core

in vec4 vColor;
in vec2 vUV;

out vec4 FragColor;

void main() {
    // Soft circular particle
    float dist = length(vUV - vec2(0.5));
    if (dist > 0.5) discard;
    float alpha = smoothstep(0.5, 0.0, dist);
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}