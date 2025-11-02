#version 460 core

out vec4 FragColor;
in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform float u_Alpha;

void main()
{
    vec4 texColor = texture(u_Texture, v_TexCoord);
    if (texColor.a < 0.1)
        discard;
    FragColor = vec4(texColor.rgb, texColor.a * u_Alpha);
}
