#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aAlpha;
layout (location = 2) in vec2 aUV;

uniform mat4 view;
uniform mat4 projection;

out float vAlpha;
out vec2  vUV;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
    vAlpha = aAlpha;
    vUV    = aUV;
}