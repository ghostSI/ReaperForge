R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  vec4 gradient = vec4(0.05, 0.05, 0.05, 0.7 - Texcoord.y);
  outColor = mix(color, vec4(gradient.rgb, 1.0), gradient.a);
}
)"
