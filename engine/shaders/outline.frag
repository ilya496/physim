#version 460 core

out vec4 FragColor;

uniform vec3 u_OutlineColor;

void main()
{
    FragColor = vec4(u_OutlineColor, 1.0);
}
