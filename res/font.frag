R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform sampler2D texture0;

void main()
{
  outColor = texture(texture0, Texcoord);
}
)"