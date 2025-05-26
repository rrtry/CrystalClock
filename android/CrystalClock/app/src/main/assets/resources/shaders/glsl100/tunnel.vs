#version 100

attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 mNormal;

varying vec2 fragTexCoord;
varying vec3 fragPosition;
varying vec4 fragColor;
varying vec3 normal;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor    = vertexColor;
	fragPosition = vec3(model * vec4(vertexPosition, 1.0));
	normal       = mat3(mNormal) * vertexNormal;
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}