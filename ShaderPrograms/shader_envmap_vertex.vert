#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 7) in mat4 InstanceMatrix;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = InstanceMatrix * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    mat3 normalMatrix = transpose(inverse(mat3(InstanceMatrix)));
    Normal = normalize(normalMatrix * aNormal);
    gl_Position = projection * view * worldPos;
}
