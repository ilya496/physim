#version 460 core

out vec4 FragColor;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoords;
in vec4 v_FragPosLightSpace;

uniform vec3 u_LightPos;
uniform vec3 u_ViewPos;
uniform vec3 u_LightColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}; 
  
uniform Material material;

uniform sampler2D u_Texture;
uniform sampler2D shadowMap;
uniform sampler2D u_NormalMap;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = 0.00005; 
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    if(projCoords.z > 1.0)
        shadow = 0.0;

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    return shadow;
}  

void main() {
    // ambient
    // vec3 ambient = u_LightColor * material.ambient * vec3(texture(u_Texture, v_TexCoords));
    vec3 ambient = u_LightColor * material.ambient;

    // diffuse 
    vec3 norm = normalize(v_Normal);
    // vec3 norm = texture(u_NormalMap, v_TexCoords).rgb;
    // norm = normalize(norm * 2.0 - 1.0);
    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = u_LightColor * (diff * material.diffuse);
    // vec3 diffuse = u_LightColor * (diff * material.diffuse) * vec3(texture(u_Texture, v_TexCoords));
    
    // specular
    vec3 viewDir = normalize(u_ViewPos - v_FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = u_LightColor * (spec * material.specular);  
        
    // shadow 
    float shadow = ShadowCalculation(v_FragPosLightSpace);

    vec3 result = (ambient + (1.0 - shadow) * (diffuse + specular)) * u_LightColor;
    FragColor = vec4(result, 1.0);
}