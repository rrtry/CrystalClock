#version 100

attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec3 vertexTangent;
attribute vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 mNormal;

varying vec2 fragTexCoord;
varying vec3 fragPosition;
varying vec3 tanFragPosition;
varying vec3 fragNormal;
varying vec4 fragColor;
varying mat3 TBN;

mat3 transpose(mat3 m) {
    return mat3(
		vec3(m[0].x, m[1].x, m[2].x),
		vec3(m[0].y, m[1].y, m[2].y),
		vec3(m[0].z, m[1].z, m[2].z)
	);
}

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