#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

in vec3 fragPosition;
in vec3 normal;

struct PointLight {

    vec3 position;  
    vec3 ambient;
    vec3 diffuse;
	
    float constant;
    float linear;
    float quadratic;
};

uniform PointLight tunlight;
uniform sampler2D texture0;
uniform float time;

uniform vec3 viewPos;
uniform vec4 colDiffuse;

out vec4 finalColor;

const vec3 mainColor = vec3(0.28, 0.19, 0.43);
const vec3 secondaryColor = vec3(0.18, 0.10, 0.32);

vec3 SampleNoise()
{
	vec2 texCoord = fragTexCoord + vec2(time, 0.0);
	float noise   = texture(texture0, texCoord).r;
	float t 	  = smoothstep(0.0, 1.0, noise);
	return mix(mainColor * 1.25, secondaryColor, t);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPosition, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPosition);
	if (dot(normal, lightDir) < 0)
	{
		normal = -normal;
	}
	
    float diff    	  = max(dot(normal, lightDir), 0.0);
    float distance    = length(light.position - fragPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
	
	vec3 noise    = SampleNoise();
    vec3 ambient  = light.ambient  * noise;
    vec3 diffuse  = light.diffuse  * diff * noise;
	
    ambient *= attenuation;
    diffuse *= attenuation;
	
    return (ambient + diffuse);
}

void main()
{
	vec3 fragVec = viewPos - fragPosition;
	float fade   = 1.0 - smoothstep(0, 100, length(fragVec));
	vec4 color   = vec4(CalcPointLight(tunlight, normalize(normal), fragPosition, normalize(fragVec)), fade);
	finalColor   = color;
}

