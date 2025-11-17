#version 460 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_Normal;

out vec3 v_Normal;
out vec3 v_WorldPos;

void main() {
    vec4 worldPos = u_Model * vec4(a_Pos, 1.0);
    v_WorldPos = worldPos.xyz;
    v_Normal = u_Normal * a_Normal;
    gl_Position = u_Projection * u_View * worldPos;
}
