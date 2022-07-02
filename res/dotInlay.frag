R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;
uniform vec4 color2;

float circle(in vec2 dist, in float radius)
{
  return 1.0 - smoothstep(radius-(radius * 0.04), radius+(radius * 0.04), dot(dist,dist)*4.0);
}

void main()
{
  vec3 col = color.rgb;
  vec3 colBorder = color2.rgb;
  
  vec2 dist = vec2((Texcoord.x - 0.5) * (16.0 / 9.0), Texcoord.y - 0.5);
  
  float a = circle(dist, 0.9);
  if (a <= 0.0)
    discard;
	
  outColor = vec4(mix(col, colBorder, 1.0 - circle(dist, 0.6)), a);
}
)"
