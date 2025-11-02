#version 460 core

out vec4 FragColor;

uniform vec4 u_Color;

void main()
{
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5)
        discard;
    FragColor = u_Color;
}