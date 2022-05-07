R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;
uniform sampler2D texture0;

void main()
{
  vec4 tex = texture(texture0, Texcoord);
  
  if (tex.a <= 0.0)
    discard;
  
  outColor = mix(color, tex, 1.0 - tex.a);
}
)"
