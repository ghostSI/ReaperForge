R"(#version 330 core

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;
uniform sampler2D texture0;

void main()
{
  vec2 texcoord = vec2(Texcoord.x , 1.0 - Texcoord.y);
  vec4 tex = texture(texture0, texcoord);
  outColor = mix(color, vec4(tex.rgb, 1.0), tex.a);
}
)"