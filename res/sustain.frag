R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  float x = abs(Texcoord.x - 0.5);
  
  vec3 mixer = color.rgb;
  vec3 white = vec3(0.9);

  float line = step(x, 0.43);
  mixer = mix(mixer, white, line);
  line = step(x, 0.28);
  mixer = mix(mixer, color.rgb, line);

  outColor = vec4(mixer, 0.65);
}
)"
