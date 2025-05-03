#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 model;

out vec2 fragTexCoord;
out vec3 fragPosition;
out vec4 fragColor;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor 	 = vertexColor;
	fragPosition = vertexPosition;
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}