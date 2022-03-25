R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;

void main()
{
  vec4 myvec = vec4(0.2, 0.2, 0.6, 1.0);
  
  myvec.a = pow(sin(Texcoord.x * 400.0), 100);
  
  outColor = myvec;
}
)"