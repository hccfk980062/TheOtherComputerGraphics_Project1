#version 330 core

in vec4 vColor;
in vec2 vUV;

uniform sampler2D uTexture;
uniform bool      uUseTexture;

out vec4 FragColor;

void main() {
    if (uUseTexture) {
        // Sample texture and multiply by vertex color (tint support)
        vec4 texColor = texture(uTexture, vUV) * vColor;
        if (texColor.a < 0.01) discard;
        FragColor = texColor;
    } else {
        // Soft circular particle using distance-based alpha falloff
        float dist = length(vUV - vec2(0.5));
        if (dist > 0.5) discard;
        float alpha = smoothstep(0.5, 0.0, dist);
        FragColor = vec4(vColor.rgb, vColor.a * alpha);
    }
}
