R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  vec4 col = color;
  
  col.a *= 0.5 * pow(2.0 * abs(Texcoord.x - 0.5), 50.5) + 0.5 * pow(1.0 - Texcoord.y, 20.5);
  col.a *= 1.0 - Texcoord.y;
  col.a *= pow(2.0 * abs(Texcoord.x - 0.5), 5.0);
  
  outColor = vec4(col.rgb, col.a);
}
)"