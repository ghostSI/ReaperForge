R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;

void main()
{
  vec4 color = vec4(0.2, 0.2, 0.6, 1.0);
  
  color.a = pow(sin(Texcoord.x * 154.0), 100);
  
  if (color.a <= 0.0)
    discard;
  
  outColor = color;
}
)"