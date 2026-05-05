#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 7) in mat4 InstanceMatrix;

uniform mat4 lightSpaceMatrix;

void main()
{
    gl_Position = lightSpaceMatrix * InstanceMatrix * vec4(aPos, 1.0);
}
