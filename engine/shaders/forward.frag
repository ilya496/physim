#version 460 core

struct Light
{
    vec3 Position;
    vec3 Direction;
    vec3 Color;
    float Intensity;
    int Type;      // 0 = Directional, 1 = Point
    float Range;
};

#define MAX_LIGHTS 32

uniform int u_LightCount;
uniform Light u_Lights[MAX_LIGHTS];

uniform vec3 u_ViewPos;

// material
uniform vec3 material_diffuseColor;
uniform vec3 material_specularColor;
uniform float material_shininess;

uniform bool material_hasDiffuseMap;
uniform sampler2D material_diffuseMap;

in VS_OUT
{
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(fs_in.Normal);
    vec3 viewDir = normalize(u_ViewPos - fs_in.WorldPos);

    vec3 albedo = material_diffuseColor;
    if (material_hasDiffuseMap)
        albedo *= texture(material_diffuseMap, fs_in.TexCoords).rgb;

    vec3 result = vec3(0.0);

    for (int i = 0; i < u_LightCount; i++)
    {
        Light light = u_Lights[i];

        vec3 lightDir;
        float attenuation = 1.0;

        if (light.Type == 0) // directional
        {
            lightDir = normalize(-light.Direction);
        }
        else // point
        {
            vec3 toLight = light.Position - fs_in.WorldPos;
            float distance = length(toLight);
            lightDir = normalize(toLight);

            attenuation = clamp(1.0 - distance / light.Range, 0.0, 1.0);
        }

        // diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * albedo * light.Color;

        // specular
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
        vec3 specular = spec * material_specularColor * light.Color;

        result += (diffuse + specular) * light.Intensity * attenuation;
    }

    // ambient
    vec3 ambient = albedo * 0.5;
    result += ambient;

    FragColor = vec4(result, 1.0);
    // FragColor = vec4(material_diffuseColor, 1.0);
}
