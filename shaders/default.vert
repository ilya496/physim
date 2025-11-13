#version 460 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoords;
out mat3 v_TBN;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_Normal;

void main() {
    vec4 worldPos = u_Model * vec4(a_Pos, 1.0);
    v_FragPos = worldPos.xyz;
    v_Normal = normalize(u_Normal * a_Normal);
    v_TexCoords = a_TexCoords;

    mat3 model3 = mat3(u_Model);
    vec3 T = normalize(model3 * a_Tangent);
    vec3 B = normalize(model3 * a_Bitangent);
    vec3 N = normalize(model3 * a_Normal);

    // Re-orthogonalize (important if T and N aren’t perfectly perpendicular)
    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);
    v_TBN = mat3(T, B, N);

    gl_Position = u_Projection * u_View * worldPos;
}
