#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float texture_info;
layout (location = 2) in vec3 aNormal;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;

void main()
{
    gl_Position = projection_matrix * (view_matrix * vec4(aPos, 1.0));
}
