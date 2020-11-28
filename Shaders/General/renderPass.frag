#version 430 core
layout(location = 0) out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    vec3 specular;
    float shininess;
}; 

struct Light {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

#define NR_POINT_LIGHTS 4

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPos;
uniform Light dirLight;
uniform Light pointLights[NR_POINT_LIGHTS];
uniform Light spotLight;
uniform Material material;

// function prototypes
vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir, vec4 color);
vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 color);
vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 color);

void main()
{    
    //FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    
    //FragColor = vec4(Normal, 1.0);
    //vec3 cam = (view * vec4(viewPos, 1.0)).xyz;

    // properties
    vec3 fragPos = FragPos;//.xyz;//gl_FragCoord.xyz;
    vec3 norm = normalize(-Normal);
    vec3 viewDir = normalize(viewPos - fragPos);
    //gl_FragDepth = dot(-fragPos + viewPos, normalize(viewDir));
   
    vec3 result = CalcDirLight(dirLight, norm, viewDir, Color);
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, fragPos, viewDir, Color);    
    result += CalcSpotLight(spotLight, norm, fragPos, viewDir, Color);    
    
    // if transparent uncomment this 
    //FragColor = vec4(result, 0.3) * Color;
    FragColor = vec4(result, 1) * Color;
    //FragColor = vec4(norm, 1);
	//FragColor = texture(material.diffuse, TexCoords);
    /**/
}

// calculates the color when using a directional light.
vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir, vec4 color)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
	vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0),  material.shininess);
    // combine results
    vec3 ambient = light.ambient * color.xyz;
    vec3 diffuse = light.diffuse * diff * color.xyz;
    vec3 specular = light.specular * spec * material.specular;
    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 color)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0),  material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient = light.ambient * color.xyz;
    vec3 diffuse = light.diffuse * diff * color.xyz;
    vec3 specular = light.specular * spec * material.specular;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 color)
{
    //vec3 lightDir = normalize(-light.direction);
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
	vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0),  material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient * color.xyz;
    vec3 diffuse = light.diffuse * diff * color.xyz;
    vec3 specular = light.specular * spec * material.specular;
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}
