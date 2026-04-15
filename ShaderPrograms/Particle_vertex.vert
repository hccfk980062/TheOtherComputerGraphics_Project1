#version 330 core

layout(location = 0) in vec2  aQuadPos;      // quad geometry
layout(location = 1) in vec3  instancePos;   // per-particle
layout(location = 2) in vec4  instanceColor;
layout(location = 3) in float instanceSize;

uniform mat4 view;
uniform mat4 projection;

out vec4 vColor;
out vec2 vUV;

void main() {
    // Billboard: extract camera right/up from view matrix
    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);

    vec3 worldPos = instancePos
                  + camRight * aQuadPos.x * instanceSize
                  + camUp    * aQuadPos.y * instanceSize;

    gl_Position = projection * view * vec4(worldPos, 1.0);
    vColor = instanceColor;
    vUV    = aQuadPos + 0.5; // 0..1
}