R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  outColor = color;
}
)"
