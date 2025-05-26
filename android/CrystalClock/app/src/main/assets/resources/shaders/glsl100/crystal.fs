#version 100

precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

varying vec3 fragPosition;
varying vec3 tanFragPosition;
varying vec3 fragNormal;
varying mat3 TBN;

struct Material {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {

    vec3 position;  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
    float constant;
    float linear;
    float quadratic;
};

uniform Material material;
uniform DirLight dirLight;

#define NR_POINT_LIGHTS 7  
uniform PointLight pointLights[NR_POINT_LIGHTS];

uniform sampler2D texture0;
uniform sampler2D normalMap;

uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec4 colDiffuse;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPosition, vec3 viewDir);

vec3 CalcRimLight(vec3 viewDir, vec3 N);
vec3 CalcReflection(vec3 N);
vec3 CalcRefraction(vec3 N);

void main()
{
    vec3 norm = texture2D(normalMap, fract(fragTexCoord)).rgb;
	norm = norm * 2.0 - 1.0;
	norm = normalize(TBN * norm);
	
	vec3 tanViewDir = normalize((TBN * viewPos) - tanFragPosition);
    vec3 viewDir    = normalize(viewPos - fragPosition);
    vec3 result     = CalcDirLight(dirLight, fragNormal, viewDir);
	
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
	{
		result += CalcPointLight(pointLights[i], norm, tanFragPosition, tanViewDir);
	}
	gl_FragColor = vec4(mix(CalcRimLight(viewDir, fragNormal), result, 0.7), 0.7);
}

vec3 CalcRimLight(vec3 viewDir, vec3 N)
{
	return vec3(1.0) * smoothstep(0.3, 0.4, pow(max(0.0, 1.0 - dot(viewDir, N)), 2.0));
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	// diffuse
	float diff = max(dot(normal, lightDir), 0.0);

	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	
	// combine results
    vec3 ambient  = light.ambient  * material.ambient;
    vec3 diffuse  = light.diffuse  * diff * material.diffuse;
    vec3 specular = light.specular * spec * material.specular;
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPosition, vec3 viewDir)
{
	vec3 lightPosition = TBN * light.position;
    vec3 lightDir = normalize(lightPosition - fragPosition);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance    = length(lightPosition - fragPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
				 
    vec3 ambient  = light.ambient  * material.ambient;
    vec3 diffuse  = light.diffuse  * diff * material.diffuse;
    vec3 specular = light.specular * spec * material.specular;
	
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
	
    return (ambient + diffuse + specular);
}


