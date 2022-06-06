R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;
uniform vec4 color2;
uniform float progress;

void main()
{
  vec4 progessColor = color;
  if (Texcoord.x < progress)
    progessColor = color2;

  vec4 gradient = vec4(0.05, 0.05, 0.05, 0.7 - Texcoord.y);
  outColor = mix(progessColor, vec4(gradient.rgb, 1.0), gradient.a);
}
)"
