R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;
uniform sampler2D texture0;

void main()
{
  vec4 tex = texture(texture0, Texcoord);
  outColor = mix(color, tex, tex.a);
}
)"