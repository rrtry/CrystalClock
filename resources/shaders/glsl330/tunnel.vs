#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 mNormal;

out vec2 fragTexCoord;
out vec3 fragPosition;
out vec4 fragColor;
out vec3 normal;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor    = vertexColor;
	fragPosition = vec3(model * vec4(vertexPosition, 1.0));
	normal       = mat3(mNormal) * vertexNormal;
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}