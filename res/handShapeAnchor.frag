R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  vec4 col = color;

  float x =  max(2.2 * (abs(Texcoord.x - 0.5) - 0.40), 0.0);
  col.a = pow(sin(x * 3.1415 * 3.0) - 0.9, 0.15);

  outColor = col;
}
)"
