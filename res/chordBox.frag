R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  vec4 col = color;

  float x =  max(max(abs(Texcoord.x - 0.5), abs(Texcoord.y - 0.5)) - 0.47, 0.0);
  col.a = pow(sin(x * 3.1415 * 6), 0.8);
  col.a += min(col.a, 1.1 - Texcoord.y);
  col.a += max(col.a, (1.0 - Texcoord.y) * 0.25);
  
  if (col.a <= 0.0)
    discard;

  outColor = col;
}
)"