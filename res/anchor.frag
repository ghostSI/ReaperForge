R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  outColor = color;
}
)"