R"(#version 300 es

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
out vec2 Texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Texcoord = texcoord;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)"
