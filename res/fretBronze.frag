R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform sampler2D texture0;

void main()
{
  vec2 texcoord = vec2(Texcoord.x + 0.2 , 1.0 - Texcoord.y);
  outColor = texture(texture0, texcoord);
}
)"