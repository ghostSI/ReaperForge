R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  vec4 col = color;
  
  col.a *= pow(0.5 * -cos(Texcoord.x * 6.366) + 0.5, 0.2);
  
  outColor = col;
}
)"
