R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;
uniform vec4 color2;

void main()
{
  float a = cos((Texcoord.x - 0.5) * 3.0);
  
  outColor = mix(color2, color, a);
}
)"
