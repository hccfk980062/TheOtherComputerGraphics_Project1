#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec4 clipSpacePos;
out vec3 worldPos;
out vec3 toCamera;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;

void main()
{
    vec4 wPos    = model * vec4(aPos, 1.0);
    worldPos     = wPos.xyz;
    toCamera     = cameraPos - wPos.xyz;
    clipSpacePos = projection * view * wPos;
    gl_Position  = clipSpacePos;
}
