#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec3 vertexTangent;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 mNormal;

out vec2 fragTexCoord;
out vec3 fragPosition;
out vec3 tanFragPosition;
out vec3 fragNormal;
out vec4 fragColor;
out mat3 TBN;

void main()
{
	fragTexCoord = vertexTexCoord * 2.0;
    fragColor    = vertexColor;
	
	vec3 vertexBinormal = cross(vertexNormal, vertexTangent);
	vec3 normal = normalize(mat3(mNormal) * vertexNormal);
	fragNormal  = normal;
	
	vec3 fragTangent = normalize(mat3(mNormal) * vertexTangent);
	fragTangent = normalize(fragTangent - dot(fragTangent, normal) * normal);
	
	vec3 fragBinormal = normalize(mat3(mNormal) * vertexBinormal);
    fragBinormal = cross(normal, fragTangent);
	TBN = transpose(mat3(fragTangent, fragBinormal, normal));
	
	fragPosition    = vec3(model * vec4(vertexPosition, 1.0));
	tanFragPosition = TBN * fragPosition;
	
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}