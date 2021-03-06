R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;
uniform sampler2D texture0;

void main()
{
  vec2 texcoord = vec2(Texcoord.x , 1.0 - Texcoord.y);
  vec4 tex = texture(texture0, texcoord);

  vec3 col = mix(color.rgb, tex.rgb, tex.a);
  
  outColor = vec4(col.rgb, color.a);
}
)"
