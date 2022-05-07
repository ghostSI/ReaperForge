R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

void main()
{
  vec3 stringColor = color.rgb * pow((0.5 * -cos(Texcoord.y * 6.366) + 0.5), 1.5);
  vec3 woundStringColor = stringColor * pow(0.2 * sin(Texcoord.x * 15000.0 + Texcoord.y * 2.2) + 0.8, 1.0);
  outColor = mix(vec4(stringColor, 1.0), vec4(woundStringColor, 1.0), color.a);
}
)"
