#version 460 core

out vec4 FragColor;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoords;
in mat3 v_TBN;

uniform vec3 u_LightPos;
uniform vec3 u_ViewPos;
uniform vec3 u_LightColor;

struct Material {
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
    sampler2D diffuseMap;
    sampler2D specularMap;
    sampler2D normalMap;
    bool hasDiffuseMap;
    bool hasSpecularMap;
    bool hasNormalMap;
}; 

uniform Material material;

void main()
{
    vec3 normal;
    if (material.hasNormalMap)
    {
        vec3 tangentNormal = texture(material.normalMap, v_TexCoords).rgb * 2.0 - 1.0;
        tangentNormal.y *= -1.0;
        normal = normalize(v_TBN * tangentNormal);
        // normal = normalize(tangentNormal);
    }
    else
    {
        normal = normalize(v_Normal);
    }

    vec3 albedo = material.diffuseColor;
    if (material.hasDiffuseMap)
        albedo *= texture(material.diffuseMap, v_TexCoords).rgb;

    vec3 specularColor = material.specularColor;
    if (material.hasSpecularMap)
        specularColor *= texture(material.specularMap, v_TexCoords).rgb;

    vec3 ambient = 0.25 * albedo;

    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    vec3 viewDir  = normalize(u_ViewPos - v_FragPos);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo;

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = spec * specularColor;

    vec3 lighting = (ambient + diffuse + specular) * u_LightColor;

    FragColor = vec4(lighting, 1.0);
}
