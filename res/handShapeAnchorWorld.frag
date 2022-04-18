R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  vec4 col = color;

  col.a = pow(sin(Texcoord.x * 154.0), 100);

  outColor = col;
}
)"