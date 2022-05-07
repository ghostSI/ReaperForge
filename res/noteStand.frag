R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  vec4 col = color;
  
  col.a *= max(0.9 - Texcoord.y, 0.0);
  col.a *= pow(1.0 - abs(Texcoord.x - 0.5), 0.5);
  col.a *= 0.6;
  
  outColor = vec4(col.rgb, col.a);
}
)"
