#version 460 core

out vec4 FragColor;
in vec3 v_WorldPos;

uniform vec3 u_Color;
uniform vec3 u_CameraPos;

uniform float u_FogDensity = 0.025;   // thickness
uniform float u_FogPower   = 1.35;     // curve shaping

void main()
{
    // distance in XZ plane (grid plane)
    float dist = length(v_WorldPos.xz - u_CameraPos.xz);

    float fog = exp(-pow(dist * u_FogDensity, u_FogPower));
    fog = clamp(fog, 0.0, 1.0);

    FragColor = vec4(u_Color, fog);

    // reduce overdraw
    if (fog < 0.01)
        discard;
}
