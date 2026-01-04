#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

out VS_OUT
{
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    vs_out.WorldPos = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    vs_out.Normal = normalize(normalMatrix * a_Normal);

    vs_out.TexCoords = a_TexCoords;

    gl_Position = u_Projection * u_View * worldPos;
}
