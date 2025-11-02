#version 460 core

layout(location = 0) in vec3 a_Pos;

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_PointSize;

void main() {
    gl_Position = u_Projection * u_View * vec4(a_Pos, 1.0);
    gl_PointSize = u_PointSize;
}