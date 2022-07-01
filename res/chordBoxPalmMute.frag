R"(#version 300 es
precision mediump float;

in vec2 Texcoord;
out vec4 outColor;
uniform vec4 color;

mat2 rotate45(){
    return mat2(cos(0.7853981),-sin(0.7853981), sin(0.7853981),cos(0.7853981));
}

void main()
{
  vec4 col = color;

  float x =  max(max(abs(Texcoord.x - 0.5), abs(Texcoord.y - 0.5)) - 0.47, 0.0);
  
  vec2 coord = Texcoord;
  coord -= vec2(0.5);
  coord = rotate45() * coord;
  coord += vec2(0.5);
  vec2 crossX = step(vec2(0.06, 0.06), abs(vec2(coord.x - 0.5, coord.y - 0.5)));
  
  col.a = pow(sin(x * 3.1415 * 6.0), 0.8);
  col.a += (1.0 - crossX.x * crossX.y) * 0.12;
  col.a += min(col.a, 1.1 - Texcoord.y);
  col.a += max(col.a, (1.0 - Texcoord.y) * 0.25);
  
  if (col.a <= 0.0)
    discard;

  outColor = col;
}
)"
