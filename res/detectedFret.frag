R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  float a = 2.0 * (0.5 - distance(Texcoord, vec2(0.5)));
  a = pow(a, 0.4);

  outColor = vec4(color.rgb, color.a * a);
}
)"
