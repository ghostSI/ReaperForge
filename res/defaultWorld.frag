#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform sampler2D texture0;

void main()
{
  vec4 texColor = texture(texture0, Texcoord);

  if(texColor.a < 0.1)
    discard;

  outColor = texColor;
}
